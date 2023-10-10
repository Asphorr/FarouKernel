#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <filesystem>

using namespace std;

class DirectoryWriter {
public:
    // Create a new directory and write data to file
    template <typename T>
    void createDirectoryAndWriteToFile(const fs::path& dirPath, const vector<T>& data) {
        try {
            // Check if directory exists
            if (!fs::exists(dirPath)) {
                // Create directory
                fs::create_directories(dirPath);
            }
            
            // Open output stream
            fstream outStream(dirPath / "file.txt", ios::out | ios::trunc);
            
            // Write data to file
            for (auto&& elem : data) {
                outStream << elem << endl;
            }
            
            // Close output stream
            outStream.close();
        } catch (exception& e) {
            cerr << "Error creating or writing to file: " << e.what() << endl;
        }
    }
    
    // Read data from file and delete it
    void readFromFileAndDeleteIt(const fs::path& filePath) {
        try {
            // Open input stream
            fstream inStream(filePath, ios::in);
            
            // Read data from file
            string content((istreambuf_iterator<char>(inStream)), istreambuf_iterator<char>());
            
            // Print data to console
            cout << "File contents:" << endl << content << endl;
            
            // Delete file
            fs::remove(filePath);
        } catch (exception& e) {
            cerr << "Error reading or deleting file: " << e.what() << endl;
        }
    }
    
    // Delete empty directory
    void deleteEmptyDirectory(const fs::path& dirPath) {
        try {
            // Check if directory is empty
            if (fs::is_empty(dirPath)) {
                // Remove directory
                fs::remove(dirPath);
            } else {
                throw runtime_error("Cannot remove non-empty directory.");
            }
        } catch (exception& e) {
            cerr << "Error removing directory: " << e.what() << endl;
        }
    }
};

int main() {
    DirectoryWriter writer;
    fs::path path{"dir"};
    vector<string> data{"Hello", "World!"};
    
    writer.createDirectoryAndWriteToFile(path, data);
    writer.readFromFileAndDeleteIt(path / "file.txt");
    writer.deleteEmptyDirectory(path);
    
    return 0;
}
