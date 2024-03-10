#ifndef DIR_FUNCTIONS_H
#define DIR_FUNCTIONS_H

#include <string>
#include <vector>
#include <filesystem>
#include <fstream>
#include <iostream>

namespace DirFunctions {

void CreateDirectoryAndWriteToFile(const std::string& dirPathStr, const std::vector<std::string>& data, std::filesystem::perms perms = std::filesystem::perms::all) {
    std::filesystem::path dirPath(dirPathStr);
    std::filesystem::create_directories(dirPath);
    std::filesystem::path filePath = dirPath / "data.txt";
    std::ofstream file(filePath, std::ios::out | std::ios::app); // Open for writing at the end of the file
    if (!file) {
        throw std::runtime_error("Failed to open file for writing: " + filePath.string());
    }
    for (const std::string& line : data) {
        file << line << '\n'; // Using '\n' for possibly better performance than std::endl
    }
    std::filesystem::permissions(filePath, perms);
}

void DeleteDirectory(const std::string& dirPathStr, bool deleteNonEmpty = false) {
    std::filesystem::path dirPath(dirPathStr);
    if (std::filesystem::exists(dirPath) && (deleteNonEmpty || std::filesystem::is_empty(dirPath))) {
        std::filesystem::remove_all(dirPath);
    }
}

void CopyDirectory(const std::string& sourcePathStr, const std::string& destinationPathStr, bool copySelf = false) {
    std::filesystem::path sourcePath(sourcePathStr);
    std::filesystem::path destinationPath(destinationPathStr);
    if (copySelf) {
        destinationPath /= sourcePath.filename();
    }
    std::filesystem::copy(sourcePath, destinationPath, std::filesystem::copy_options::recursive);
}

std::vector<std::string> GetAllFilesInDir(const std::string& dirPathStr, bool fullPath = false) {
    std::filesystem::path dirPath(dirPathStr);
    std::vector<std::string> files;
    for (const auto& entry : std::filesystem::directory_iterator(dirPath)) {
        if (entry.is_regular_file()) {
            files.push_back(fullPath ? entry.path().string() : entry.path().filename().string());
        }
    }
    return files;
}

bool IsDirectoryEmpty(const std::string& dirPathStr) {
    return std::filesystem::is_empty(dirPathStr);
}

bool DoesFileExist(const std::string& filePathStr) {
    return std::filesystem::exists(filePathStr) && std::filesystem::is_regular_file(filePathStr);
}

std::string ReadFromFile(const std::string& filePathStr) {
    std::ifstream file(filePathStr);
    if (!file) {
        throw std::runtime_error("Failed to open file for reading: " + filePathStr);
    }
    return std::string((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
}

} // namespace DirFunctions

#endif // DIR_FUNCTIONS_H
