#ifndef DIR_FUNCTIONS_H
#define DIR_FUNCTIONS_H

#include <string>
#include <vector>
#include <filesystem>
#include <fstream>
#include <algorithm>

namespace DirFunctions {

void createDirectoryAndWriteToFile(const std::filesystem::path& dirPath, const std::vector<std::string>& data) {
    if (!std::filesystem::exists(dirPath)) {
        std::filesystem::create_directory(dirPath);
    }
    std::filesystem::path filePath = dirPath / "data.txt";
    std::ofstream file(filePath);
    for (const auto& line : data) {
        file << line << std::endl;
    }
    file.close();
}

void readFromFileAndDeleteIt(const std::filesystem::path& filePath) {
    if (std::filesystem::exists(filePath)) {
        std::ifstream file(filePath);
        std::string line;
        while (std::getline(file, line)) {
            std::cout << line << std::endl;
        }
        file.close();
        std::filesystem::remove(filePath);
    }
}

void deleteEmptyDirectory(const std::filesystem::path& dirPath) {
    if (std::filesystem::exists(dirPath) && std::filesystem::is_empty(dirPath)) {
        std::filesystem::remove(dirPath);
    }
}

bool hasFilesInDir(const std::filesystem::path& dirPath) {
    if (std::filesystem::exists(dirPath) && std::filesystem::is_directory(dirPath)) {
        for (const auto& entry : std::filesystem::directory_iterator(dirPath)) {
            if (entry.is_regular_file()) {
                return true;
            }
        }
    }
    return false;
}

void copyDirectory(const std::filesystem::path& sourcePath, const std::filesystem::path& destinationPath) {
    if (std::filesystem::exists(sourcePath) && std::filesystem::is_directory(sourcePath)) {
        std::filesystem::copy(sourcePath, destinationPath, std::filesystem::copy_options::recursive);
    }
}

std::vector<std::filesystem::path> getAllFilesInDir(const std::filesystem::path& dirPath) {
    std::vector<std::filesystem::path> files;
    if (std::filesystem::exists(dirPath) && std::filesystem::is_directory(dirPath)) {
        for (const auto& entry : std::filesystem::directory_iterator(dirPath)) {
            if (entry.is_regular_file()) {
                files.push_back(entry.path());
            }
        }
    }
    return files;
}

std::vector<std::filesystem::path> getAllDirectoriesInDir(const std::filesystem::path& dirPath) {
    std::vector<std::filesystem::path> directories;
    if (std::filesystem::exists(dirPath) && std::filesystem::is_directory(dirPath)) {
        for (const auto& entry : std::filesystem::directory_iterator(dirPath)) {
            if (entry.is_directory()) {
                directories.push_back(entry.path());
            }
        }
    }
    return directories;
}

} // namespace DirFunctions

#endif // DIR_FUNCTIONS_H
