#ifndef DATA_PROCESSOR_HPP
#define DATA_PROCESSOR_HPP

#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <cxxopts.hpp>
#include <toml.hpp>
#include <tbb/parallel_for_each.h>
#include <spdlog/spdlog.h>

class ConfigError : public std::runtime_error {
public:
    explicit ConfigError(const std::string& msg) : std::runtime_error(msg) {}
};

class FileError : public std::runtime_error {
public:
    explicit FileError(const std::string& msg) : std::runtime_error(msg) {}
};

class Config {
public:
    std::string inputFile;
    std::string outputFile;
    std::string delimiter = " ";
    std::string commentStyle = "#";
    std::string outputFormat = "keyvalue";
    int chunkSize = 1024;
    bool useParallelProcessing = false;

    void load(const std::string& configFile, const cxxopts::ParseResult& cliArgs);
    void validate() const;
};

class InputParser {
public:
    explicit InputParser(const Config& config);
    std::vector<std::pair<std::string, int>> parse();

private:
    const Config& config_;
    std::pair<std::string, int> parseLine(const std::string& line) const;
};

class DataProcessor {
public:
    explicit DataProcessor(const Config& config);
    void process(std::vector<std::pair<std::string, int>>& data);

private:
    const Config& config_;
    void processDataPoint(std::pair<std::string, int>& dataPoint);
};

class OutputGenerator {
public:
    explicit OutputGenerator(const Config& config);
    void generate(const std::vector<std::pair<std::string, int>>& data);

private:
    const Config& config_;
    void generateJSON(const std::vector<std::pair<std::string, int>>& data, std::ostream& os);
    void generateCSV(const std::vector<std::pair<std::string, int>>& data, std::ostream& os);
    void generateKeyValue(const std::vector<std::pair<std::string, int>>& data, std::ostream& os);
};

class Main {
public:
    int run(int argc, char* argv[]);

private:
    cxxopts::ParseResult parseCLI(int argc, char* argv[]);
    Config loadConfig(const std::string& configFile, const cxxopts::ParseResult& cliArgs);
    void processData(const Config& config);
};

#endif // DATA_PROCESSOR_HPP
