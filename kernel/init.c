#include <iostream>
#include <fstream>
#include <string>
#include <unordered_map>
#include <algorithm>
#include <execution>

using DataMap = std::unordered_map<std::string, int>;

DataMap readDataFromFile(const std::string& filename) {
    std::ifstream input(filename);
    if (!input) {
        std::cerr << "Could not open file: " << filename << '\n';
        return {};
    }

    DataMap data;
    std::string key;
    int value;
    while (input >> key >> value) {
        data[std::move(key)] = value;
    }
    return data;
}

void processData(DataMap& data) {
    std::for_each(std::execution::par_unseq, data.begin(), data.end(),
        [](auto& pair) {
            if (pair.second > 10) {
                pair.second *= 2;
                pair.first += "_doubled";
            } else {
                pair.second /= 2;
                pair.first += "_halved";
            }
        });
}

void writeOutputToFile(const std::string& filename, const DataMap& data) {
    std::ofstream output(filename);
    if (!output) {
        std::cerr << "Could not open file: " << filename << '\n';
        return;
    }

    for (const auto& [key, value] : data) {
        output << key << ": " << value << '\n';
    }
}

int main() {
    auto data = readDataFromFile("input.txt");
    processData(data);
    writeOutputToFile("output.txt", data);
}
