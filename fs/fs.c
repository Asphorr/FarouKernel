#include <iostream>
#include <string>
#include <vector>
#include <filesystem>

// Define a namespace for the application
namespace myapp {

// Define a class for the FileSystem
class FileSystem {
public:
    // Constructor
    explicit FileSystem() : root_(nullptr), current_(root_) {}

    // Destructor
    ~FileSystem() {
        delete root_;
    }

    // Copy constructor
    FileSystem(const FileSystem& other) : root_(other.root_), current_(current_) {}

    // Move constructor
    FileSystem(FileSystem&& other) noexcept : root_(other.root_), current_(current_) {
        other.root_ = nullptr;
    }

    // Assignment operator
    FileSystem& operator=(const FileSystem& other) {
        if (this != &other) {
            delete root_;
            root_ = other.root_;
            current_ = current_;
        }
        return *this;
    }

    // Move assignment operator
    FileSystem& operator=(FileSystem&& other) noexcept {
        if (this != &other) {
            delete root_;
            root_ = other.root_;
            current_ = current_;
            other.root_ = nullptr;
        }
        return *this;
    }

    // Create a directory
    bool mkdir(const std::string& path) {
        try {
            std::filesystem::create_directory(path);
            return true;
        } catch (...) {
            return false;
        }
    }

    // Remove a directory
    bool rmdir(const std::string& path) {
        try {
            std::filesystem::remove(path);
            return true;
        } catch (...) {
            return false;
        }
    }

    // Get the contents of a directory
    std::vector<std::string> ls(const std::string& path) const {
        try {
            std::vector<std::string> files;
            for (auto& entry : std::filesystem::recursive_directory_iterator(path)) {
                files.emplace_back(entry.path().filename());
            }
            return files;
        } catch (...) {
            return {};
        }
    }

private:
    // Root directory
    std::shared_ptr<Directory> root_;

    // Current working directory
    Directory* current_;
};

} // namespace myapp

int main() {
    myapp::FileSystem fs;

    // Create some directories
    fs.mkdir("mydir");
    fs.mkdir("mydir/subdir1");
    fs.mkdir("mydir/subdir2");

    // Print the contents of the root directory
    std::cout << "Root directory:\n";
    for (auto& dir : fs.ls("/")) {
        std::cout << "\t" << dir << '\n';
    }

    // Change into the subdirectory
    fs.cd("mydir/subdir1");

    // Print the contents of the subdirectory
    std::cout << "Subdirectory:\n";
    for (auto& dir : fs.ls(".")) {
        std::cout << "\t" << dir << '\n';
    }

    // Go back up one level
    fs.cd("..");

    // Delete the subdirectories
    fs.rmdir("mydir/subdir1");
    fs.rmdir("mydir/subdir2");

    // Print the contents of the root directory again
    std::cout << "Root directory after deletion:\n";
    for (auto& dir : fs.ls("/")) {
        std::cout << "\t" << dir << '\n';
    }

    return 0;
}
