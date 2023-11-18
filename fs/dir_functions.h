#ifndef DIR_FUNCTIONS_H
#define DIR_FUNCTIONS_H

#include <string>
#include <vector>
#include <filesystem>
#include <fstream>
#include <algorithm>

namespace DirFunctions {

void createDirectoryAndWriteToFile(const std::filesystem::path& dirPath, const std::vector<std::string>& data, std::filesystem::perms perms = std::filesystem::perms::all) {
    if (!std::filesystem::exists(dirPath)) {
        if (!std::filesystem::create_directory(dirPath)) {
            throw std::runtime_error("Failed to create directory");
        }
    }
    std::filesystem::path filePath = dirPath / "data.txt";
    std::ofstream file(filePath, std::ios::app);
    if (!file) {
        throw std::runtime_error("Failed to open file for writing");
    }
    for (const auto& line : data) {
        file << line << std::endl;
    }
    file.close();
    std::filesystem::permissions(filePath, perms);
}

// ...

void deleteEmptyDirectory(const std::filesystem::path& dirPath, bool deleteNonEmpty = false) {
    if (std::filesystem::exists(dirPath) && (!deleteNonEmpty || std::filesystem::is_empty(dirPath))) {
        if (!std::filesystem::remove(dirPath)) {
            throw std::runtime_error("Failed to delete directory");
        }
    }
}

// ...

void copyDirectory(const std::filesystem::path& sourcePath, const std::filesystem::path& destinationPath, bool copySelf = false) {
    if (std::filesystem::exists(sourcePath) && std::filesystem::is_directory(sourcePath)) {
        if (copySelf) {
            if (!std::filesystem::copy(sourcePath, destinationPath / sourcePath.filename(), std::filesystem::copy_options::recursive)) {
                throw std::runtime_error("Failed to copy directory");
            }
        } else {
            if (!std::filesystem::copy(sourcePath, destinationPath, std::filesystem::copy_options::recursive)) {
                throw std::runtime_error("Failed to copy directory");
            }
        }
    }
}

// ...

std::vector<std::filesystem::path> getAllFilesInDir(const std::filesystem::path& dirPath, bool fullPath = false) {
    std::vector<std::filesystem::path> files;
    if (std::filesystem::exists(dirPath) && std::filesystem::is_directory(dirPath)) {
        for (const auto& entry : std::filesystem::directory_iterator(dirPath)) {
            if (entry.is_regular_file()) {
                if (fullPath) {
                    files.push_back(entry.path());
                } else {
                    files.push_back(entry.path().filename());
                }
            }
        }
    }
    return files;
}

// ...

bool isDirectoryEmpty(const std::filesystem::path& dirPath) {
    return std::filesystem::is_empty(dirPath);
}

bool doesFileExist(const std::filesystem::path& filePath) {
    return std::filesystem::exists(filePath) && std::filesystem::is_regular_file(filePath);
}

std::string readFromFile(const std::filesystem::path& filePath) {
    std::ifstream file(filePath);
    if (!file) {
        throw std::runtime_error("Failed to open file for reading");
    }
    std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    file.close();
    return content;
}

} // namespace DirFunctions

#endif // DIR_FUNCTIONS_H
