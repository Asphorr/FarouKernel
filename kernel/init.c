#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <map>
#include <cmath>
#include <numeric>
#include <execution>

// Define a struct to store key-value pairs
struct KeyValuePair {
    std::string key;
    int value;
};

// Implement a custom comparator for sorting by value
struct ValueComparator {
    bool operator()(const KeyValuePair& lhs, const KeyValuePair& rhs) const {
        return lhs.value < rhs.value;
    }
};

// Function to read data from a file and store it in a vector of key-value pairs
std::vector<KeyValuePair> readDataFromFile(const char* filename) {
    std::ifstream input(filename);
    std::vector<KeyValuePair> data;
    
    // Read data from file and push it onto the vector
    std::string line;
    while (std::getline(input, line)) {
        auto keyValuePair = splitLineIntoKeyAndValue(line);
        data.push_back({keyValuePair.first, stoi(keyValuePair.second)});
    }
    
    return data;
}

// Function to process data stored in a vector of key-value pairs
void processData(std::vector<KeyValuePair>& data) {
    // Sort the data based on the values
    std::sort(data.begin(), data.end(), ValueComparator());
    
    // Iterate over the sorted data and update the values
    for (size_t i = 0; i < data.size(); ++i) {
        auto& pair = data[i];
        
        // Check if the current value is greater than 10
        if (pair.value > 10) {
            // Multiply the value by 2
            pair.value *= 2;
            
            // Update the corresponding key
            pair.key += "_doubled";
        } else {
            // Divide the value by 2
            pair.value /= 2;
            
            // Update the corresponding key
            pair.key += "_halved";
        }
    }
}

// Function to write processed data to a file
void writeOutputToFile(const char* filename, const std::vector<KeyValuePair>& data) {
    std::ofstream output(filename);
    
    // Write the processed data to the file
    for (const auto& pair : data) {
        output << pair.key << ": " << pair.value << "\n";
    }
}

int main() {
    // Read data from a file and store it in a vector of key-value pairs
    std::vector<KeyValuePair> data = readDataFromFile("input.txt");
    
    // Process the data
    processData(data);
    
    // Write the processed data to a file
    writeOutputToFile("output.txt", data);
    
    return 0;
}
