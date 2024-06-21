#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <map>
#include <cmath>
#include <numeric>
#include <execution>

struct KeyValuePair {
   std::string key;
   int value;
};

struct ValueComparator {
   bool operator()(const KeyValuePair& lhs, const KeyValuePair& rhs) const {
       return lhs.value < rhs.value;
   }
};

std::map<std::string, int> readDataFromFile(const char* filename) {
   std::ifstream input(filename);
   if (!input.is_open()) {
       std::cerr << "Could not open file: " << filename << std::endl;
       return {};
   }

   std::map<std::string, int> data;
   std::string line;
   while (std::getline(input, line)) {
       auto keyValuePair = splitLineIntoKeyAndValue(line);
       data[keyValuePair.first] = stoi(keyValuePair.second);
   }
   input.close();
   return data;
}

void processData(std::map<std::string, int>& data) {
   std::transform(data.begin(), data.end(), data.begin(),
       [](auto& pair) {
           if (pair.second > 10) {
               pair.second *= 2;
               pair.first += "_doubled";
           } else {
               pair.second /= 2;
               pair.first += "_halved";
           }
           return pair;
       });
}

void writeOutputToFile(const char* filename, const std::map<std::string, int>& data) {
   std::ofstream output;
   output.open(filename);
   if (!output.is_open()) {
       std::cerr << "Could not open file: " << filename << std::endl;
       return;
   }

   for (const auto& pair : data) {
       output << pair.first << ": " << pair.second << "\n";
   }
   output.close();
}

int main() {
   std::map<std::string, int> data = readDataFromFile("input.txt");
   processData(data);
   writeOutputToFile("output.txt", data);
   return 0;
}
