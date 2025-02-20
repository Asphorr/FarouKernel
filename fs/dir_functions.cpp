// dir_functions.h
#pragma once
#include <filesystem>
#include <chrono>
#include <optional>
#include <string>
#include <system_error>
#include <thread>

namespace fs = std::filesystem;

namespace DirFunctions {

class Error : public std::runtime_error {
public:
    explicit Error(std::string msg) : std::runtime_error(std::move(msg)) {}
};

// Configuration struct for move operations
struct MoveOptions {
    bool preserve_permissions = true;
    bool overwrite_existing = false;
    unsigned retry_attempts = 3;
    std::chrono::milliseconds retry_delay{100};
};

// RAII wrapper for filesystem operations
class FileOperationGuard {
public:
    FileOperationGuard(const fs::path& src, const fs::path& dst);
    ~FileOperationGuard() noexcept;
    void commit() noexcept { committed_ = true; }

private:
    fs::path source_;
    fs::path temp_backup_;
    bool committed_ = false;
};

// Move file with advanced options and rollback capability
[[nodiscard]] fs::path moveFile(const fs::path& source, 
                              const fs::path& destination,
                              const MoveOptions& options = {});

// Move directory with advanced options and rollback capability
[[nodiscard]] fs::path moveDirectory(const fs::path& source, 
                                   const fs::path& destination,
                                   const MoveOptions& options = {});

// Get file size with caching option
[[nodiscard]] std::optional<uintmax_t> getFileSize(const fs::path& path, 
                                                 bool use_cache = false) noexcept;

// Get directory size with parallel processing option
[[nodiscard]] std::optional<uintmax_t> getDirectorySize(const fs::path& path,
                                                      bool parallel = true) noexcept;

// Get last write time with high precision
[[nodiscard]] std::optional<std::chrono::system_clock::time_point> 
    getLastWriteTime(const fs::path& path) noexcept;

// Set last write time with validation
[[nodiscard]] bool setLastWriteTime(const fs::path& path, 
                                  std::chrono::system_clock::time_point newTime);

} // namespace DirFunctions

// dir_functions.cpp
#include "dir_functions.h"
#include <mutex>
#include <shared_mutex>
#include <unordered_map>
#include <execution>

namespace DirFunctions {

namespace {
// Thread-safe cache for file sizes
class SizeCache {
public:
    std::optional<uintmax_t> get(const fs::path& path) const {
        std::shared_lock lock(mutex_);
        auto it = cache_.find(path);
        return (it != cache_.end()) ? std::make_optional(it->second) : std::nullopt;
    }

    void set(const fs::path& path, uintmax_t size) {
        std::unique_lock lock(mutex_);
        cache_[path] = size;
    }

private:
    mutable std::shared_mutex mutex_;
    std::unordered_map<fs::path, uintmax_t> cache_;
} size_cache;

void throwIfError(const std::error_code& ec, const char* msg) {
    if (ec) throw Error(std::string(msg) + ": " + ec.message());
}

bool tryOperation(auto&& operation, const MoveOptions& options) {
    std::error_code ec;
    for (unsigned i = 0; i < options.retry_attempts; ++i) {
        if (operation(ec)) return true;
        if (i + 1 < options.retry_attempts) {
            std::this_thread::sleep_for(options.retry_delay);
        }
    }
    throwIfError(ec, "Operation failed after retries");
    return false;
}

void moveAcrossFs(const fs::path& src, const fs::path& dst, const MoveOptions& options) {
    fs::copy_options copy_opts = fs::copy_options::recursive | 
                                fs::copy_options::copy_symlinks;
    if (options.preserve_permissions) {
        copy_opts |= fs::copy_options::preserve_permissions;
    }
    if (options.overwrite_existing) {
        copy_opts |= fs::copy_options::overwrite_existing;
    }

    tryOperation([&](std::error_code& ec) {
        fs::copy(src, dst, copy_opts, ec);
        return !ec;
    }, options);

    tryOperation([&](std::error_code& ec) {
        fs::remove_all(src, ec);
        return !ec;
    }, options);
}
} // anonymous namespace

FileOperationGuard::FileOperationGuard(const fs::path& src, const fs::path& dst) 
    : source_(src) {
    if (fs::exists(dst)) {
        temp_backup_ = dst.string() + ".bak";
        fs::rename(dst, temp_backup_);
    }
}

FileOperationGuard::~FileOperationGuard() noexcept {
    if (!committed_ && !temp_backup_.empty()) {
        try {
            if (fs::exists(source_)) fs::remove_all(source_);
            fs::rename(temp_backup_, source_);
        } catch (...) {
            // Log error in production code
        }
    }
}

fs::path moveFile(const fs::path& source, const fs::path& destination, 
                 const MoveOptions& options) {
    if (!fs::exists(source)) throw Error("Source file does not exist");

    FileOperationGuard guard(source, destination);
    std::error_code ec;

    auto tryRename = [&](std::error_code& ec) {
        fs::rename(source, destination, ec);
        return !ec;
    };

    try {
        if (!tryOperation(tryRename, options)) {
            moveAcrossFs(source, destination, options);
        }
        guard.commit();
    } catch (...) {
        throw;
    }
    return destination;
}

fs::path moveDirectory(const fs::path& source, const fs::path& destination, 
                      const MoveOptions& options) {
    if (!fs::exists(source)) throw Error("Source directory does not exist");
    
    FileOperationGuard guard(source, destination);
    std::error_code ec;

    auto tryRename = [&](std::error_code& ec) {
        fs::rename(source, destination, ec);
        return !ec;
    };

    try {
        if (!tryOperation(tryRename, options)) {
            moveAcrossFs(source, destination, options);
        }
        guard.commit();
    } catch (...) {
        throw;
    }
    return destination;
}

std::optional<uintmax_t> getFileSize(const fs::path& path, bool use_cache) noexcept {
    if (use_cache) {
        if (auto cached = size_cache.get(path)) {
            return cached;
        }
    }

    std::error_code ec;
    auto size = fs::file_size(path, ec);
    if (!ec && use_cache) {
        size_cache.set(path, size);
    }
    return ec ? std::nullopt : std::make_optional(size);
}

std::optional<uintmax_t> getDirectorySize(const fs::path& path, bool parallel) noexcept {
    std::error_code ec;
    if (!fs::exists(path, ec) || ec) return std::nullopt;

    uintmax_t total = 0;
    std::vector<fs::path> files;

    // Collect files first
    for (const auto& entry : fs::recursive_directory_iterator(
            path, fs::directory_options::skip_permission_denied, ec)) {
        if (ec) continue;
        if (entry.is_regular_file(ec) && !ec) {
            files.push_back(entry.path());
        }
    }

    if (parallel) {
        total = std::reduce(std::execution::par, files.begin(), files.end(), uintmax_t{0},
            [](uintmax_t sum, const fs::path& p) {
                if (auto size = getFileSize(p)) return sum + *size;
                return sum;
            });
    } else {
        for (const auto& file : files) {
            if (auto size = getFileSize(file)) total += *size;
        }
    }
    return total;
}

std::optional<std::chrono::system_clock::time_point> 
getLastWriteTime(const fs::path& path) noexcept {
    std::error_code ec;
    auto ftime = fs::last_write_time(path, ec);
    if (ec) return std::nullopt;

    try {
        return std::chrono::clock_cast<std::chrono::system_clock>(ftime);
    } catch (const std::chrono::clock_cast_error&) {
        return std::nullopt;
    }
}

bool setLastWriteTime(const fs::path& path, 
                     std::chrono::system_clock::time_point newTime) {
    if (!fs::exists(path)) throw Error("Path does not exist");

    std::error_code ec;
    auto fsTime = std::chrono::clock_cast<fs::file_time_type::clock>(newTime);
    fs::last_write_time(path, fsTime, ec);
    throwIfError(ec, "Failed to set write time");
    return true;
}

} // namespace DirFunctions
