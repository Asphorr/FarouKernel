#include "dir_functions.h"
#include <iostream>
#include <fstream>
#include <vector>
#include <filesystem>

using namespace std;
namespace fs = std::filesystem;

namespace DirFunctions {

    // ... existing functions ...

    void moveFile(const fs::path& sourcePath, const fs::path& destinationPath) {
        if (!fs::exists(sourcePath)) {
            throw std::invalid_argument("Source file does not exist");
        }
        if (fs::exists(destinationPath)) {
            throw std::invalid_argument("Destination file already exists");
        }
        fs::rename(sourcePath, destinationPath);
    }

    void moveDirectory(const fs::path& sourcePath, const fs::path& destinationPath) {
        if (!fs::exists(sourcePath)) {
            throw std::invalid_argument("Source directory does not exist");
        }
        if (fs::exists(destinationPath)) {
            throw std::invalid_argument("Destination directory already exists");
        }
        fs::rename(sourcePath, destinationPath);
    }

    void getFileSize(const fs::path& filePath) {
        if (!fs::exists(filePath)) {
            throw std::invalid_argument("File does not exist");
        }
        cout << "File size: " << fs::file_size(filePath) << " bytes" << endl;
    }

    void getDirectorySize(const fs::path& dirPath) {
        if (!fs::exists(dirPath)) {
            throw std::invalid_argument("Directory does not exist");
        }
        uintmax_t totalSize = 0;
        for (const auto & entry : fs::recursive_directory_iterator(dirPath)) {
            if (entry.is_regular_file()) {
                totalSize += fs::file_size(entry.path());
            }
        }
        cout << "Directory size: " << totalSize << " bytes" << endl;
    }

    void getFilePermissions(const fs::path& filePath) {
        if (!fs::exists(filePath)) {
            throw std::invalid_argument("File does not exist");
        }
        fs::perms p = fs::status(filePath).permissions();
        cout << ((p & fs::perms::owner_read) != fs::perms::none ? "r" : "-")
             << ((p & fs::perms::owner_write) != fs::perms::none ? "w" : "-")
             << ((p & fs::perms::owner_exec) != fs::perms::none ? "x" : "-")
             << ((p & fs::perms::group_read) != fs::perms::none ? "r" : "-")
             << ((p & fs::perms::group_write) != fs::perms::none ? "w" : "-")
             << ((p & fs::perms::group_exec) != fs::perms::none ? "x" : "-")
             << ((p & fs::perms::others_read) != fs::perms::none ? "r" : "-")
             << ((p & fs::perms::others_write) != fs::perms::none ? "w" : "-")
             << ((p & fs::perms::others_exec) != fs::perms::none ? "x" : "-")
             << endl;
    }

    void setFilePermissions(const fs::path& filePath, fs::perms p) {
        if (!fs::exists(filePath)) {
            throw std::invalid_argument("File does not exist");
        }
        fs::permissions(filePath, p);
    }

    void setDirectoryPermissions(const fs::path& dirPath, fs::perms p) {
        if (!fs::exists(dirPath)) {
            throw std::invalid_argument("Directory does not exist");
        }
        fs::permissions(dirPath, p);
    }

    void getLastWriteTime(const fs::path& filePath) {
        if (!fs::exists(filePath)) {
            throw std::invalid_argument("File does not exist");
        }
        auto ftime = fs::last_write_time(filePath);
        time_t cftime = decltype(ftime)::clock::to_time_t(ftime);
        cout << "Last write time: " << std::asctime(std::localtime(&cftime)) << endl;
    }

    void setLastWriteTime(const fs::path& filePath, time_t newTime) {
        if (!fs::exists(filePath)) {
            throw std::invalid_argument("File does not exist");
        }
        fs::last_write_time(filePath, newTime);
    }

}
