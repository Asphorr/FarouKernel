#include <iostream>
#include <fstream>
#include <filesystem>
#include <string>
#include <vector>

// Define some useful aliases
using Path = std::filesystem::path;
using FileStream = std::basic_ifstream<char>;
using StringVector = std::vector<std::string>;

// Function to create a new directory and write to a file within it
void createDirectoryAndWriteToFile(Path& path, StringVector& data) {
    // Create the directory if it doesn't exist
    if (!fs::exists(path)) {
        fs::create_directory(path);
    }
    
    // Open the file for writing
    FileStream outfile(path / "file.txt", std::ios::out | std::ios::trunc);
    
    // Write the data to the file
    for (const auto& line : data) {
        outfile << line << '\n';
    }
    
    // Close the file
    outfile.close();
}

// Function to read from a file and delete it
void readFromFileAndDeleteIt(Path& path) {
    // Read the entire file into memory
    FileStream infile(path / "file.txt");
    std::string content((std::istreambuf_iterator<char>(infile)), std::istreambuf_iterator<char>());
    
    // Print the contents of the file
    std::cout << "File contents:\n" << content << "\n";
    
    // Delete the file
    fs::remove(path / "file.txt");
}

// Function to delete an empty directory
void deleteEmptyDirectory(Path& path) {
    // Check if the directory exists and is empty
    if (fs::is_empty(path)) {
        // Remove the directory
        fs::remove(path);
    } else {
        throw std::runtime_error("Cannot remove non-empty directory.");
    }
}

int main() {
    // Set up the paths and data
    Path path{"dir"};
    StringVector data{"Hello", "World!"};
    
    // Call the functions
    createDirectoryAndWriteToFile(path, data);
    readFromFileAndDeleteIt(path);
    deleteEmptyDirectory(path);
    
    return 0;
}
