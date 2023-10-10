#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <filesystem>

namespace fs = std::filesystem;

void createDirectoryAndWriteToFile(const fs::path& dirPath, const std::vector<std::string>& data) {
    try {
        if (!fs::exists(dirPath)) {
            fs::create_directory(dirPath);
        }
        
        std::ofstream outStream(dirPath / "file.txt", std::ios::out | std::ios::trunc);
        
        for (const auto& line : data) {
            outStream << line << '\n';
        }
        
        outStream.close();
    } catch (const std::exception& e) {
        std::cerr << "Error creating or writing to file: " << e.what() << '\n';
    }
}

void readFromFileAndDeleteIt(const fs::path& filePath) {
    try {
        std::ifstream inStream(filePath, std::ios::in);
        
        std::string content((std::istreambuf_iterator<char>(inStream)), std::istreambuf_iterator<char>());
        
        std::cout << "File contents:\n" << content << "\n";
        
        fs::remove(filePath);
    } catch (const std::exception& e) {
        std::cerr << "Error reading or deleting file: " << e.what() << '\n';
    }
}

void deleteEmptyDirectory(const fs::path& dirPath) {
    try {
        if (fs::is_empty(dirPath)) {
            fs::remove(dirPath);
        } else {
            throw std::runtime_error("Cannot remove non-empty directory.");
        }
    } catch (const std::exception& e) {
        std::cerr << "Error removing directory: " << e.what() << '\n';
    }
}

int main() {
    fs::path path{"dir"};
    std::vector<std::string> data{"Hello", "World!"};
    
    createDirectoryAndWriteToFile(path, data);
    readFromFileAndDeleteIt(path / "file.txt");
    deleteEmptyDirectory(path);
    
    return 0;
}
