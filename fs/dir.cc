#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <filesystem>

namespace fs = std::filesystem;

// Creates a new directory at the specified path and writes the given data to a file inside it
void createDirectoryAndWriteToFile(const fs::path& dirPath, const std::vector<std::string>& data) {
    try {
        // Create the directory if it doesn't exist
        if (!fs::exists(dirPath)) {
            fs::create_directory(dirPath);
        }

        // Write the data to a file inside the directory
        std::ofstream outStream(dirPath / "file.txt", std::ios::out | std::ios::trunc);
        for (const auto& line : data) {
            outStream << line << '\n';
        }
        outStream.close();
    } catch (const std::exception& e) {
        std::cerr << "Failed to create directory or write to file: " << e.what() << '\n';
    }
}

// Reads the contents of the specified file and deletes it afterward
void readFromFileAndDeleteIt(const fs::path& filePath) {
    try {
        // Read the contents of the file
        std::ifstream inStream(filePath, std::ios::in);
        std::string content((std::istreambuf_iterator<char>(inStream)), std::istreambuf_iterator<char>());
        inStream.close();

        // Print the contents of the file to standard output
        std::cout << "File contents:\n" << content << "\n";

        // Delete the file
        fs::remove(filePath);
    } catch (const std::exception& e) {
        std::cerr << "Failed to read or delete file: " << e.what() << '\n';
    }
}

// Deletes the specified empty directory
void deleteEmptyDirectory(const fs::path& dirPath) {
    try {
        // Check if the directory is empty
        if (fs::is_empty(dirPath)) {
            // Delete the directory
            fs::remove(dirPath);
        } else {
            throw std::runtime_error("Cannot remove non-empty directory.");
        }
    } catch (const std::exception& e) {
        std::cerr << "Failed to remove directory: " << e.what() << '\n';
    }
}

// Returns true if the specified directory exists and contains files, false otherwise
bool hasFilesInDir(const fs::path& dirPath) {
    bool result = false;
    try {
        // Open the directory and check if it contains any files
        fs::directory_iterator endIter;
        for (auto iter = fs::begin(dirPath); iter != endIter; ++iter) {
            if (fs::is_regular_file(*iter)) {
                result = true;
                break;
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "Error checking directory: " << e.what() << '\n';
    }
    return result;
}

int main() {
    // Define the path to the directory and the data to be written to the file
    fs::path path{"dir"};
    std::vector<std::string> data{"Hello", "World!"};

    // Call the function to create the directory and write the data to a file
    createDirectoryAndWriteToFile(path, data);

    // Call the function to read the contents of the file and delete it
    readFromFileAndDeleteIt(path / "file.txt");

    // If the directory still exists and contains files, delete it
    if (hasFilesInDir(path)) {
        deleteEmptyDirectory(path);
    }

    return 0;
}
