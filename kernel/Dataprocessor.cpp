#include "DataProcessor.hpp"

void Config::load(const std::string& configFile, const cxxopts::ParseResult& cliArgs) {
    if (!configFile.empty()) {
        try {
            auto data = toml::parse(configFile);
            inputFile = toml::find_or(data, "input_file", "");
            outputFile = toml::find_or(data, "output_file", "");
            delimiter = toml::find_or(data, "delimiter", " ");
            commentStyle = toml::find_or(data, "comment_style", "#");
            outputFormat = toml::find_or(data, "output_format", "keyvalue");
            chunkSize = toml::find_or(data, "chunk_size", 1024);
            useParallelProcessing = toml::find_or(data, "use_parallel_processing", false);
        } catch (const std::exception& e) {
            spdlog::warn("Failed to load TOML config: {}", e.what());
        }
    }

    if (cliArgs.count("input")) inputFile = cliArgs["input"].as<std::string>();
    if (cliArgs.count("output")) outputFile = cliArgs["output"].as<std::string>();
    if (cliArgs.count("delimiter")) delimiter = cliArgs["delimiter"].as<std::string>();
    if (cliArgs.count("comment")) commentStyle = cliArgs["comment"].as<std::string>();
    if (cliArgs.count("format")) outputFormat = cliArgs["format"].as<std::string>();
    if (cliArgs.count("chunk")) chunkSize = cliArgs["chunk"].as<int>();
    if (cliArgs.count("parallel")) useParallelProcessing = cliArgs["parallel"].as<bool>();
}

void Config::validate() const {
    if (inputFile.empty()) throw ConfigError("Input file path cannot be empty");
    if (outputFile.empty()) throw ConfigError("Output file path cannot be empty");
    if (chunkSize <= 0) throw ConfigError("Chunk size must be positive");
}

InputParser::InputParser(const Config& config) : config_(config) {}

std::vector<std::pair<std::string, int>> InputParser::parse() {
    std::ifstream input(config_.inputFile);
    if (!input) throw FileError("Could not open input file: " + config_.inputFile);

    std::vector<std::pair<std::string, int>> data;
    std::string line;
    while (std::getline(input, line)) {
        if (line.empty() || line[0] == config_.commentStyle[0]) continue;
        auto parsed = parseLine(line);
        if (!parsed.first.empty()) data.push_back(parsed);
    }
    return data;
}

std::pair<std::string, int> InputParser::parseLine(const std::string& line) const {
    std::istringstream iss(line);
    std::string key, valueStr;
    if (std::getline(iss, key, config_.delimiter[0]) && std::getline(iss, valueStr)) {
        try {
            int value = std::stoi(valueStr);
            return {key, value};
        } catch (const std::invalid_argument&) {
            spdlog::warn("Invalid value in line: {}", line);
        } catch (const std::out_of_range&) {
            spdlog::warn("Value out of range in line: {}", line);
        }
    }
    return {};
}

DataProcessor::DataProcessor(const Config& config) : config_(config) {}

void DataProcessor::process(std::vector<std::pair<std::string, int>>& data) {
    if (config_.useParallelProcessing) {
        tbb::parallel_for_each(data.begin(), data.end(), [this](auto& pair) {
            processDataPoint(pair);
        });
    } else {
        for (auto& pair : data) {
            processDataPoint(pair);
        }
    }
}

void DataProcessor::processDataPoint(std::pair<std::string, int>& dataPoint) {
    if (dataPoint.second > 10) {
        dataPoint.second *= 2;
    } else {
        dataPoint.second /= 2;
    }
}

OutputGenerator::OutputGenerator(const Config& config) : config_(config) {}

void OutputGenerator::generate(const std::vector<std::pair<std::string, int>>& data) {
    std::ofstream output(config_.outputFile);
    if (!output) throw FileError("Could not open output file: " + config_.outputFile);

    if (config_.outputFormat == "json") {
        generateJSON(data, output);
    } else if (config_.outputFormat == "csv") {
        generateCSV(data, output);
    } else {
        generateKeyValue(data, output);
    }
}

void OutputGenerator::generateJSON(const std::vector<std::pair<std::string, int>>& data, std::ostream& os) {
    os << "{\n";
    for (size_t i = 0; i < data.size(); ++i) {
        os << "  \"" << data[i].first << "\": " << data[i].second;
        if (i < data.size() - 1) os << ",";
        os << "\n";
    }
    os << "}\n";
}

void OutputGenerator::generateCSV(const std::vector<std::pair<std::string, int>>& data, std::ostream& os) {
    for (const auto& pair : data) {
        os << pair.first << "," << pair.second << "\n";
    }
}

void OutputGenerator::generateKeyValue(const std::vector<std::pair<std::string, int>>& data, std::ostream& os) {
    for (const auto& pair : data) {
        os << pair.first << ": " << pair.second << "\n";
    }
}

cxxopts::ParseResult Main::parseCLI(int argc, char* argv[]) {
    cxxopts::Options options("DataProcessor", "Process key-value data from files");
    options.add_options()
        ("i,input", "Input file", cxxopts::value<std::string>())
        ("o,output", "Output file", cxxopts::value<std::string>())
        ("d,delimiter", "Delimiter", cxxopts::value<std::string>())
        ("c,comment", "Comment character", cxxopts::value<std::string>())
        ("f,format", "Output format (keyvalue, json, csv)", cxxopts::value<std::string>())
        ("s,chunk", "Chunk size", cxxopts::value<int>())
        ("p,parallel", "Enable parallel processing", cxxopts::value<bool>()->default_value("false"))
        ("config", "Config file", cxxopts::value<std::string>())
        ("h,help", "Print usage");
    return options.parse(argc, argv);
}

Config Main::loadConfig(const std::string& configFile, const cxxopts::ParseResult& cliArgs) {
    Config config;
    config.load(configFile, cliArgs);
    config.validate();
    return config;
}

void Main::processData(const Config& config) {
    InputParser parser(config);
    auto data = parser.parse();

    DataProcessor processor(config);
    processor.process(data);

    OutputGenerator generator(config);
    generator.generate(data);
}

int Main::run(int argc, char* argv[]) {
    try {
        auto cliArgs = parseCLI(argc, argv);
        if (cliArgs.count("help")) {
            std::cout << "Usage: DataProcessor [options]\n"
                      << "Options:\n"
                      << "  --input <file>    Input file path\n"
                      << "  --output <file>   Output file path\n"
                      << "  --delimiter <char> Delimiter character\n"
                      << "  --comment <char>  Comment character\n"
                      << "  --format <format> Output format (keyvalue, json, csv)\n"
                      << "  --chunk <size>    Chunk size for processing\n"
                      << "  --parallel        Enable parallel processing\n"
                      << "  --config <file>   Config file path\n";
            return 0;
        }

        std::string configFile = cliArgs.count("config") ? cliArgs["config"].as<std::string>() : "config.toml";
        Config config = loadConfig(configFile, cliArgs);
        processData(config);
        spdlog::info("Processing completed successfully.");
        return 0;
    } catch (const std::exception& e) {
        spdlog::error("Error: {}", e.what());
        return 1;
    }
}

int main(int argc, char* argv[]) {
    spdlog::set_level(spdlog::level::info); // Set logging level
    Main app;
    return app.run(argc, argv);
}
