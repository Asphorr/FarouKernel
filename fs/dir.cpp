#include "dir_functions.h"
#include <iostream>
#include <fstream>
#include <vector>
#include <filesystem>

using namespace std;
namespace fs = std::filesystem;

namespace DirFunctions {

 void createDirectoryAndWriteToFile(const fs::path& dirPath, const vector<string>& data) {
   if (!fs::exists(dirPath)) {
       fs::create_directories(dirPath);
   }
   ofstream file(dirPath / "output.txt");
   for (const auto& line : data) {
       file << line << "\n";
   }
   file.close();
 }

 void readFromFileAndDeleteIt(const fs::path& filePath) {
   ifstream file(filePath);
   string line;
   while (getline(file, line)) {
       cout << line << "\n";
   }
   file.close();
   fs::remove(filePath);
 }

 void deleteEmptyDirectory(const fs::path& dirPath) {
   if (fs::is_empty(dirPath)) {
       fs::remove(dirPath);
   }
 }

 bool hasFilesInDir(const fs::path& dirPath) {
   bool hasFiles = false;
   for (const auto & entry : fs::directory_iterator(dirPath)) {
       if (entry.is_regular_file()) {
           hasFiles = true;
           break;
       }
   }
   return hasFiles;
 }

}
