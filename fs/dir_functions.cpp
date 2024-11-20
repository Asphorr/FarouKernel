#include "dir_functions.h"
#include <filesystem>
#include <optional>
#include <chrono>
#include <string>
#include <stdexcept>

namespace fs = std::filesystem;

namespace DirFunctions {

class Error : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};

fs::path moveFile(const fs::path& source, const fs::path& destination) {
    if (!fs::exists(source)) throw Error("Source file does not exist");
    if (fs::exists(destination)) throw Error("Destination file already exists");
    fs::rename(source, destination);
    return destination;
}

fs::path moveDirectory(const fs::path& source, const fs::path& destination) {
    if (!fs::exists(source)) throw Error("Source directory does not exist");
    if (fs::exists(destination)) throw Error("Destination directory already exists");
    fs::rename(source, destination);
    return destination;
}

std::optional<uintmax_t> getFileSize(const fs::path& path) {
    try {
        return fs::exists(path) ? std::optional<uintmax_t>(fs::file_size(path)) : std::nullopt;
    } catch (const fs::filesystem_error&) {
        return std::nullopt;
    }
}

std::optional<uintmax_t> getDirectorySize(const fs::path& path) {
    if (!fs::exists(path)) return std::nullopt;
    
    uintmax_t total = 0;
    for (const auto& entry : fs::recursive_directory_iterator(path)) {
        if (entry.is_regular_file()) {
            if (auto size = getFileSize(entry.path())) {
                total += *size;
            }
        }
    }
    return total;
}

std::string getFilePermissions(const fs::path& path) {
    if (!fs::exists(path)) throw Error("File does not exist");
    
    const fs::perms p = fs::status(path).permissions();
    const char* rwx = "rwx";
    std::string result;
    result.reserve(9);
    
    for (int i = 0; i < 3; ++i) {
        for (int j = 0; j < 3; ++j) {
            result += (p & fs::perms((1 << (8-i*3-j)))) ? rwx[j] : '-';
        }
    }
    return result;
}

bool setFilePermissions(const fs::path& path, fs::perms p) {
    if (!fs::exists(path)) throw Error("File does not exist");
    fs::permissions(path, p);
    return true;
}

bool setDirectoryPermissions(const fs::path& path, fs::perms p) {
    if (!fs::exists(path)) throw Error("Directory does not exist");
    fs::permissions(path, p);
    return true;
}

std::optional<std::chrono::system_clock::time_point> getLastWriteTime(const fs::path& path) {
    try {
        return fs::exists(path) ? std::optional<std::chrono::system_clock::time_point>(fs::last_write_time(path)) : std::nullopt;
    } catch (const fs::filesystem_error&) {
        return std::nullopt;
    }
}

bool setLastWriteTime(const fs::path& path, std::chrono::system_clock::time_point newTime) {
    if (!fs::exists(path)) throw Error("File does not exist");
    fs::last_write_time(path, newTime);
    return true;
}

} // namespace DirFunctions
