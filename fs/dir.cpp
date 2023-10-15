#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <filesystem>

using namespace std;
namespace fs = std::filesystem;

// Creates a new directory at the specified path and writes the given data to a file inside it
void createDirectoryAndWriteToFile(const fs::path& dirPath, const vector<string>& data) {
    try {
        // Create the directory if it doesn't exist
        if (!fs::exists(dirPath)) {
            fs::create_directory(dirPath);
        }

        // Write the data to a file inside the directory
        ofstream outStream(dirPath / "file.txt", ios::out | ios::trunc);
        for (const string& line : data) {
            outStream << line << endl;
        }
        outStream.close();
    } catch (const exception& e) {
        cerr << "Failed to create directory or write to file: " << e.what() << endl;
    }
}

// Reads the contents of the specified file and deletes it afterward
void readFromFileAndDeleteIt(const fs::path& filePath) {
    try {
        // Read the contents of the file
        ifstream inStream(filePath, ios::in);
        string content((istream_iterator<char>(inStream)), istream_iterator<char>());
        inStream.close();

        // Print the contents of the file to standard output
        cout << "File contents:" << endl << content << endl;

        // Delete the file
        fs::remove(filePath);
    } catch (const exception& e) {
        cerr << "Failed to read or delete file: " << e.what() << endl;
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
            throw runtime_error("Cannot remove non-empty directory.");
        }
    } catch (const exception& e) {
        cerr << "Failed to remove directory: " << e.what() << endl;
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
    } catch (const exception& e) {
        cerr << "Error checking directory: " << e.what() << endl;
    }
    return result;
}

int main() {
    // Define the path to the directory and the data to be written to the file
    fs::path dirPath{"dir"};
    vector<string> data{"Hello", "World!"};

    // Call the function to create the directory and write the data to a file
    createDirectoryAndWriteToFile(dirPath, data);

    // Call the function to read the contents of the file and delete it
    readFromFileAndDeleteIt(dirPath / "file.txt");

    // If the directory still exists and contains files, delete it
    if (hasFilesInDir(dirPath)) {
        deleteEmptyDirectory(dirPath);
    }

    return 0;
}
