#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <filesystem>
#include <cstdlib>

class Directory {
public:
    explicit Directory(const std::string& name) : name_(name), parent_(nullptr) {}
    ~Directory() {
        for (auto& child : children_) {
            delete child;
        }
    }

    void addChild(Directory* child) {
        children_.emplace_back(child);
    }

    void removeChild(const std::string& name) {
        for (size_t i = 0; i < children_.size(); ++i) {
            if (children_[i]->getName() == name) {
                delete children_[i];
                children_.erase(children_.begin() + i);
                break;
            }
        }
    }

    Directory* findChildByName(const std::string& name) {
        for (auto& child : children_) {
            if (child->getName() == name) {
                return child;
            }
        }
        return nullptr;
    }

    std::vector<std::string> listFiles() const {
        std::vector<std::string> result;
        for (auto& child : children_) {
            result.insert(result.end(), child->listFiles().begin(), child->listFiles().end());
        }
        return result;
    }

private:
    std::string name_;
    Directory* parent_;
    std::vector<Directory*> children_;
};

class FileSystem {
public:
    FileSystem() : root_(new Directory(".")), current_(root_) {}
    ~FileSystem() {
        delete root_;
    }

    void cd(const std::string& path) {
        current_ = root_->findChildByName(path).get();
    }

    void pwd() {
        std::cout << current_->getName() << '\n';
    }

    void mkdir(const std::string& path) {
        auto dir = new Directory(path);
        root_->addChild(dir);
        current_ = dir;
    }

    void rmdir(const std::string& path) {
        root_->removeChild(path);
    }

    void touch(const std::string& filename) {
        std::ofstream out(filename);
        out.close();
    }

    void cat(const std::string& filename) {
        std::ifstream in(filename);
        while (!in.eof()) {
            char buffer[BUFSIZ];
            size_t bytesRead = fread(buffer, sizeof(char), BUFSIZ, in.rdbuf());
            std::cout.write(buffer, bytesRead);
        }
        in.close();
    }

    void echo(const std::string& text) {
        std::ofstream out(text);
        out.close();
    }

private:
    Directory* root_;
    Directory* current_;
};

int main() {
    FileSystem fs;

    // Create some directories
    fs.mkdir("mydir");
    fs.mkdir("mydir/subdir1");
    fs.mkdir("mydir/subdir2");

    // Print the contents of the root directory
    fs.pwd();
    for (auto& dir : fs.ls("/")) {
        std::cout << "\t" << dir << '\n';
    }

    // Change into the subdirectory
    fs.cd("mydir/subdir1");

    // Print the contents of the subdirectory
    fs.pwd();
    for (auto& dir : fs.ls(".")) {
        std::cout << "\t" << dir << '\n';
    }

    // Go back up one level
    fs.cd("..");

    // Delete the subdirectories
    fs.rmdir("mydir/subdir1");
    fs.rmdir("mydir/subdir2");

    // Print the contents of the root directory again
    fs.pwd();
    for (auto& dir : fs.ls("/")) {
        std::cout << "\t" << dir << '\n';
    }

    return 0;
}
