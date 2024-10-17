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

#include <tbb/parallel_for_each.h>
#include <tbb/concurrent_unordered_map.h>
#include <spdlog/spdlog.h>
#include <cxxopts.hpp>
#include <toml.hpp>
#include <doctest/doctest.h>

using DataMap = tbb::concurrent_unordered_map<std::string, int>;

struct Config {
    std::string inputFile;
    std::string outputFile;
    int reserveSize;
    bool useParallelProcessing;
};

void initializeLogger() {
    spdlog::set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%^%l%$] %v");
    spdlog::set_level(spdlog::level::debug);
}

Config loadConfig(const std::string& configFile) {
    auto data = toml::parse(configFile);
    Config config;
    config.inputFile = toml::find<std::string>(data, "input_file");
    config.outputFile = toml::find<std::string>(data, "output_file");
    config.reserveSize = toml::find<int>(data, "reserve_size");
    config.useParallelProcessing = toml::find<bool>(data, "use_parallel_processing");
    return config;
}

DataMap readDataFromFile(const std::filesystem::path& filepath, int reserveSize) {
    std::ifstream input(filepath);
    if (!input) {
        spdlog::error("Could not open file: {}", filepath.string());
        throw std::runtime_error("File open error");
    }

    std::vector<std::pair<std::string, int>> rawData;
    rawData.reserve(reserveSize);

    std::string line;
    while (std::getline(input, line)) {
        std::istringstream iss(line);
        std::string key;
        int value;
        if (!(iss >> key >> value)) {
            spdlog::error("Invalid data format in file: {}", filepath.string());
            throw std::runtime_error("Data format error");
        }
        if (value < std::numeric_limits<int>::min() || value > std::numeric_limits<int>::max()) {
            spdlog::error("Value out of range in file: {}", filepath.string());
            throw std::runtime_error("Value range error");
        }
        rawData.emplace_back(std::move(key), value);
    }

    DataMap data;
    data.reserve(rawData.size());

    tbb::parallel_for_each(rawData.begin(), rawData.end(),
        [&data](const auto& pair) {
            data.insert(pair);
        });

    return data;
}

void processDataParallel(DataMap& data, const std::function<void(std::string&, int&)>& processFunc) {
    tbb::parallel_for_each(data.begin(), data.end(),
        [&processFunc](auto& pair) {
            processFunc(const_cast<std::string&>(pair.first), pair.second);
        });
}

void processDataSequential(DataMap& data, const std::function<void(std::string&, int&)>& processFunc) {
    for (auto& pair : data) {
        processFunc(const_cast<std::string&>(pair.first), pair.second);
    }
}

void writeOutputToFile(const std::filesystem::path& filepath, const DataMap& data) {
    std::ofstream output(filepath);
    if (!output) {
        spdlog::error("Could not open file: {}", filepath.string());
        throw std::runtime_error("File open error");
    }

    for (const auto& [key, value] : data) {
        output << key << ": " << value << '\n';
    }
}

int main(int argc, char* argv[]) {
    try {
        initializeLogger();

        cxxopts::Options options("DataProcessor", "Process key-value data from files");
        options.add_options()
            ("c,config", "Config file path", cxxopts::value<std::string>()->default_value("config.toml"))
            ("h,help", "Print usage");

        auto result = options.parse(argc, argv);

        if (result.count("help")) {
            std::cout << options.help() << std::endl;
            return 0;
        }

        std::string configFile = result["config"].as<std::string>();
        Config config = loadConfig(configFile);

        spdlog::info("Starting data processing");
        auto data = readDataFromFile(config.inputFile, config.reserveSize);
        
        auto processFunc = [](std::string& key, int& value) {
            if (value > 10) {
                value *= 2;
                key += "_doubled";
            } else {
                value /= 2;
                key += "_halved";
            }
        };

        if (config.useParallelProcessing) {
            processDataParallel(data, processFunc);
        } else {
            processDataSequential(data, processFunc);
        }

        writeOutputToFile(config.outputFile, data);
        spdlog::info("Processing completed successfully");
    } catch (const std::exception& e) {
        spdlog::error("Error: {}", e.what());
        return 1;
    }
    return 0;
