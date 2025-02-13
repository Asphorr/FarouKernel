// dir_functions.cpp
#include "dir_functions.h"

namespace DirFunctions {

namespace {
    void handleFilesystemError(const std::error_code& ec, const char* msg) {
        if (ec) throw Error(std::string(msg) + ": " + ec.message());
    }

    void moveAcrossFs(const fs::path& src, const fs::path& dst) {
        std::error_code ec;
        fs::copy(src, dst, fs::copy_options::recursive | fs::copy_options::copy_symlinks, ec);
        handleFilesystemError(ec, "Copy failed");

        fs::remove_all(src, ec);
        handleFilesystemError(ec, "Remove source failed");
    }
}

fs::path moveFile(const fs::path& source, const fs::path& destination) {
    std::error_code ec;
    
    if (!fs::exists(source, ec) || ec) 
        throw Error("Source file does not exist");
    
    try {
        fs::rename(source, destination);
    } 
    catch (const fs::filesystem_error&) {
        moveAcrossFs(source, destination);
    }
    return destination;
}

fs::path moveDirectory(const fs::path& source, const fs::path& destination) {
    std::error_code ec;
    
    if (!fs::exists(source, ec) || ec)
        throw Error("Source directory does not exist");
    
    try {
        fs::rename(source, destination);
    }
    catch (const fs::filesystem_error&) {
        moveAcrossFs(source, destination);
    }
    return destination;
}

std::optional<uintmax_t> getFileSize(const fs::path& path) noexcept {
    std::error_code ec;
    auto size = fs::file_size(path, ec);
    return ec ? std::nullopt : std::make_optional(size);
}

std::optional<uintmax_t> getDirectorySize(const fs::path& path) {
    std::error_code ec;
    if (!fs::exists(path, ec) || ec) return std::nullopt;

    uintmax_t total = 0;
    for (const auto& entry : fs::recursive_directory_iterator(path, 
        fs::directory_options::skip_permission_denied, ec))
    {
        if (ec) continue;
        if (entry.is_regular_file(ec)) {
            if (auto size = getFileSize(entry.path())) {
                total += *size;
            }
        }
    }
    return total;
}

std::optional<std::chrono::system_clock::time_point> getLastWriteTime(const fs::path& path) noexcept {
    std::error_code ec;
    auto ftime = fs::last_write_time(path, ec);
    if (ec) return std::nullopt;
    
    try {
        return std::chrono::clock_cast<std::chrono::system_clock>(ftime);
    }
    catch (...) {
        return std::nullopt;
    }
}

bool setLastWriteTime(const fs::path& path, std::chrono::system_clock::time_point newTime) {
    std::error_code ec;
    auto newTimeFs = fs::file_time_type::clock::from_sys(newTime);
    fs::last_write_time(path, newTimeFs, ec);
    handleFilesystemError(ec, "Set write time failed");
    return true;
}

} // namespace DirFunctions
