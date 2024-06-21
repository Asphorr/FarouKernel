#include "dir_functions.h"
#include <iostream>
#include <fstream>
#include <vector>
#include <filesystem>
#include <optional>
#include <chrono>
#include <string>
#include <stdexcept>

using namespace std;
namespace fs = std::filesystem;

namespace DirFunctions {

    struct Error : runtime_error {
        explicit Error(const string& msg) : runtime_error(msg) {}
    };

    fs::path moveFile(const fs::path& sourcePath, const fs::path& destinationPath) {
        if (!fs::exists(sourcePath)) {
            throw Error("Source file does not exist");
        }
        if (fs::exists(destinationPath)) {
            throw Error("Destination file already exists");
        }
        fs::rename(sourcePath, destinationPath);
        return destinationPath;
    }

    fs::path moveDirectory(const fs::path& sourcePath, const fs::path& destinationPath) {
        if (!fs::exists(sourcePath)) {
            throw Error("Source directory does not exist");
        }
        if (fs::exists(destinationPath)) {
            throw Error("Destination directory already exists");
        }
        fs::rename(sourcePath, destinationPath);
        return destinationPath;
    }

    std::optional<uintmax_t> getFileSize(const fs::path& filePath) {
        if (!fs::exists(filePath)) {
            return std::nullopt;
        }
        try {
            return fs::file_size(filePath);
        } catch (const filesystem_error& e) {
            return std::nullopt;
        }
    }

    std::optional<uintmax_t> getDirectorySize(const fs::path& dirPath) {
        if (!fs::exists(dirPath)) {
            return std::nullopt;
        }
        uintmax_t totalSize = 0;
        for (const auto & entry : fs::recursive_directory_iterator(dirPath)) {
            if (entry.is_regular_file()) {
                auto fileSize = getFileSize(entry.path());
                if (fileSize) {
                    totalSize += fileSize.value();
                }
            }
        }
        return totalSize;
    }

    std::string getFilePermissions(const fs::path& filePath) {
        if (!fs::exists(filePath)) {
            throw Error("File does not exist");
        }
        fs::perms p = fs::status(filePath).permissions();
        string permissions = "";
        permissions += (p & fs::perms::owner_read) != fs::perms::none ? "r" : "-";
        permissions += (p & fs::perms::owner_write) != fs::perms::none ? "w" : "-";
        permissions += (p & fs::perms::owner_exec) != fs::perms::none ? "x" : "-";
        permissions += (p & fs::perms::group_read) != fs::perms::none ? "r" : "-";
        permissions += (p & fs::perms::group_write) != fs::perms::none ? "w" : "-";
        permissions += (p & fs::perms::group_exec) != fs::perms::none ? "x" : "-";
        permissions += (p & fs::perms::others_read) != fs::perms::none ? "r" : "-";
        permissions += (p & fs::perms::others_write) != fs::perms::none ? "w" : "-";
        permissions += (p & fs::perms::others_exec) != fs::perms::none ? "x" : "-";
        return permissions;
    }

    bool setFilePermissions(const fs::path& filePath, fs::perms p) {
        if (!fs::exists(filePath)) {
            throw Error("File does not exist");
        }
        fs::permissions(filePath, p);
        return true;
    }

    bool setDirectoryPermissions(const fs::path& dirPath, fs::perms p) {
        if (!fs::exists(dirPath)) {
            throw Error("Directory does not exist");
        }
        fs::permissions(dirPath, p);
        return true;
    }

    std::optional<std::chrono::system_clock::time_point> getLastWriteTime(const fs::path& filePath) {
        if (!fs::exists(filePath)) {
            return std::nullopt;
        }
        try {
            return fs::last_write_time(filePath);
        } catch (const filesystem_error& e) {
            return std::nullopt;
        }
    }

    bool setLastWriteTime(const fs::path& filePath, std::chrono::system_clock::time_point newTime) {
        if (!fs::exists(filePath)) {
            throw Error("File does not exist");
        }
        fs::last_write_time(filePath, newTime);
        return true;
    }

}
