#include <iostream>
#include <fstream>
#include <string>
#include <algorithm>
#include <filesystem>
#include <functional>
#include <stdexcept>
#include <limits>
#include <sstream>
#include <vector>
#include <memory>
#include <chrono>  // Added for timing

#include <tbb/parallel_for_each.h>
#include <tbb/concurrent_unordered_map.h>
#include <spdlog/spdlog.h>
#include <cxxopts.hpp>
#include <toml.hpp>
#include <doctest/doctest.h>

namespace {
    using DataMap = tbb::concurrent_unordered_map<std::string, int>;

    // Custom exceptions for better error handling
    class ConfigError : public std::runtime_error {
        using std::runtime_error::runtime_error;
    };

    class FileError : public std::runtime_error {
        using std::runtime_error::runtime_error;
    };

    class DataError : public std::runtime_error {
        using std::runtime_error::runtime_error;
    };
}

class Config {
public:
    std::string inputFile;
    std::string outputFile;
    int reserveSize;
    bool useParallelProcessing;

    void validate() const {
        if (inputFile.empty()) {
            throw ConfigError("Input file path cannot be empty");
        }
        if (outputFile.empty()) {
            throw ConfigError("Output file path cannot be empty");
        }
        if (reserveSize <= 0) {
            throw ConfigError("Reserve size must be positive");
        }
    }
};

class DataProcessor {
public:
    explicit DataProcessor(const Config& config) : config_(config) {
        initializeLogger();
    }

    void process() {
        spdlog::info("Starting data processing");
        auto data = readDataFromFile();
        processData(data);
        writeOutputToFile(data);
        spdlog::info("Processing completed successfully");
    }

private:
    const Config& config_;
    static constexpr size_t MAX_LINE_LENGTH = 1024;

    static void initializeLogger() {
        static bool initialized = false;
        if (!initialized) {
            spdlog::set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%^%l%$] %v");
            spdlog::set_level(spdlog::level::debug);
            initialized = true;
        }
    }

    DataMap readDataFromFile() const {
        if (!std::filesystem::exists(config_.inputFile)) {
            throw FileError("Input file does not exist: " + config_.inputFile);
        }

        std::ifstream input(config_.inputFile);
        if (!input) {
            throw FileError("Could not open input file: " + config_.inputFile);
        }

        std::vector<std::pair<std::string, int>> rawData;
        rawData.reserve(static_cast<size_t>(config_.reserveSize));

        std::string line;
        size_t lineNum = 0;
        while (std::getline(input, line)) {
            ++lineNum;
            if (line.length() > MAX_LINE_LENGTH) {
                throw DataError("Line " + std::to_string(lineNum) + " exceeds maximum length");
            }
            if (line.empty() || line[0] == '#') {
                continue;  // Skip empty lines and comments
            }
            processLine(line, lineNum, rawData);
        }

        return createDataMap(rawData);
    }

    static void processLine(const std::string& line, size_t lineNum,
                           std::vector<std::pair<std::string, int>>& rawData) {
        std::istringstream iss(line);
        std::string key;
        int value;

        if (!(iss >> key >> value)) {
            throw DataError("Invalid data format at line " + std::to_string(lineNum));
        }

        if (key.empty()) {
            throw DataError("Empty key at line " + std::to_string(lineNum));
        }
        if (value < std::numeric_limits<int>::min() || value > std::numeric_limits<int>::max()) {
            throw DataError("Value out of range at line " + std::to_string(lineNum));
        }

        rawData.emplace_back(std::move(key), value);
    }

    static DataMap createDataMap(const std::vector<std::pair<std::string, int>>& rawData) {
        DataMap data;
        data.reserve(rawData.size());

        tbb::parallel_for_each(rawData.begin(), rawData.end(),
            [&data](const auto& pair) {
                if (!data.insert(pair).second) {
                    spdlog::warn("Duplicate key found: {}", pair.first);
                }
            });

        return data;
    }

    void processData(DataMap& data) const {
        // Simplified processFunc to only modify the value
        auto processFunc = [](int& value) {
            if (value > 10) {
                value *= 2;
            } else {
                value /= 2;
            }
        };

        if (config_.useParallelProcessing) {
            processDataParallel(data, processFunc);
        } else {
            processDataSequential(data, processFunc);
        }
    }

    static void processDataParallel(DataMap& data,
                                   const std::function<void(int&)>& processFunc) {
        tbb::parallel_for_each(data.begin(), data.end(),
            [&processFunc](auto& pair) {
                processFunc(pair.second);  // Directly modify the value
            });
    }

    static void processDataSequential(DataMap& data,
                                     const std::function<void(int&)>& processFunc) {
        for (auto& pair : data) {
            processFunc(pair.second);  // Directly modify the value
        }
    }

    void writeOutputToFile(const DataMap& data) const {
        std::filesystem::path outputPath(config_.outputFile);
        std::filesystem::create_directories(outputPath.parent_path());

        std::ofstream output(outputPath);
        if (!output) {
            throw FileError("Could not open output file: " + config_.outputFile);
        }

        output.exceptions(std::ofstream::badbit | std::ofstream::failbit);

        try {
            for (const auto& [key, value] : data) {
                output << key << ": " << value << '\n';
            }
        } catch (const std::ios_base::failure& e) {
            throw FileError("Failed to write to output file: " + std::string(e.what()));
        }
    }
};

Config loadConfig(const std::string& configFile) {
    if (!std::filesystem::exists(configFile)) {
        throw ConfigError("Config file does not exist: " + configFile);
    }

    try {
        auto data = toml::parse(configFile);
        Config config;
        config.inputFile = toml::find<std::string>(data, "input_file");
        config.outputFile = toml::find<std::string>(data, "output_file");
        config.reserveSize = toml::find<int>(data, "reserve_size");
        config.useParallelProcessing = toml::find<bool>(data, "use_parallel_processing");

        config.validate();
        return config;
    } catch (const toml::exception& e) {
        throw ConfigError("Failed to parse config file: " + std::string(e.what()));
    }
}

int main(int argc, char* argv[]) {
    try {
        cxxopts::Options options("DataProcessor", "Process key-value data from files");
        options.add_options()
            ("c,config", "Config file path", cxxopts::value<std::string>()->default_value("config.toml"))
            ("h,help", "Print usage");

        auto result = options.parse(argc, argv);

        if (result.count("help")) {
            std::cout << options.help() << std::endl;
            return 0;
        }

        const auto configFile = result["config"].as<std::string>();
        const auto config = loadConfig(configFile);

        DataProcessor processor(config);

        // Measure and log processing time
        auto start = std::chrono::high_resolution_clock::now();
        processor.process();
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
        spdlog::info("Processing took {} ms", duration);

        return 0;
    } catch (const ConfigError& e) {
        spdlog::error("Configuration error: {}", e.what());
        return 1;
    } catch (const FileError& e) {
        spdlog::error("File error: {}", e.what());
        return 2;
    } catch (const DataError& e) {
        spdlog::error("Data error: {}", e.what());
        return 3;
    } catch (const std::exception& e) {
        spdlog::error("Unexpected error: {}", e.what());
        return 4;
    }
}
