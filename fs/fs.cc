#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <filesystem>
#include <memory>
#include <utility>

using namespace std;

struct FileSystem {
    unique_ptr<Directory> root_;
    Directory* current_;

    void mkdir(const string& path) {
        try {
            auto dir = std::make_unique<Directory>(path);
            root_.reset();
            current_ = dir.get();
        } catch (exception& e) {
            cerr << "Error creating directory: " << e.what() << endl;
        }
    }

    void rmdir(const string& path) {
        try {
            auto dir = std::make_unique<Directory>(path);
            root_.release();
            current_ = nullptr;
        } catch (exception& e) {
            cerr << "Error removing directory: " << e.what() << endl;
        }
    }

    vector<string> ls(const string& path) const {
        try {
            vector<string> files;
            for (auto& entry : std::filesystem::recursive_directory_iterator(path)) {
                files.push_back(entry.path().filename());
            }
            return files;
        } catch (exception& e) {
            cerr << "Error listing directory: " << e.what() << endl;
            return {};
        }
    }
};

int main() {
    FileSystem fs;

    // Create some directories
    fs.mkdir("mydir");
    fs.mkdir("mydir/subdir1");
    fs.mkdir("mydir/subdir2");

    // Print the contents of the root directory
    cout << "Root directory:" << endl;
    for (auto& dir : fs.ls("/")) {
        cout << "\t" << dir << endl;
    }

    // Change into the subdirectory
    fs.current_ = fs.root_->findChildByName("subdir1").get();

    // Print the contents of the subdirectory
    cout << "Subdirectory:" << endl;
    for (auto& dir : fs.ls(".")) {
        cout << "\t" << dir << endl;
    }

    // Go back up one level
    fs.current_ = fs.root_->parent();

    // Delete the subdirectories
    fs.rmdir("mydir/subdir1");
    fs.rmdir("mydir/subdir2");

    // Print the contents of the root directory again
    cout << "Root directory after deletion:" << endl;
    for (auto& dir : fs.ls("/")) {
        cout << "\t" << dir << endl;
    }

    return 0;
}
