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

 void copyDirectory(const fs::path& sourcePath, const fs::path& destinationPath) {
  if (!fs::exists(sourcePath)) {
      throw std::invalid_argument("Source path does not exist");
  }
  if (fs::exists(destinationPath)) {
      throw std::invalid_argument("Destination path already exists");
  }
  fs::create_directories(destinationPath);
  for (const auto & entry : fs::directory_iterator(sourcePath)) {
      fs::copy(entry.path(), destinationPath / entry.path().filename(), fs::copy_options::recursive);
  }
 }

 vector<fs::path> getAllFilesInDir(const fs::path& dirPath) {
  vector<fs::path> files;
  for (const auto & entry : fs::directory_iterator(dirPath)) {
      if (entry.is_regular_file()) {
          files.push_back(entry.path());
      }
  }
  return files;
 }

 vector<fs::path> getAllDirectoriesInDir(const fs::path& dirPath) {
  vector<fs::path> directories;
  for (const auto & entry : fs::directory_iterator(dirPath)) {
      if (entry.is_directory()) {
          directories.push_back(entry.path());
      }
  }
  return directories;
 }

}
