// dir_functions.h
#pragma once

#include <chrono>
#include <filesystem>
#include <optional>
#include <string>
#include <stdexcept>
#include <system_error>

namespace fs = std::filesystem;

namespace DirFunctions {

class Error : public std::runtime_error {
public:
    using runtime_error::runtime_error;
};

[[nodiscard]] fs::path moveFile(const fs::path& source, const fs::path& destination);
[[nodiscard]] fs::path moveDirectory(const fs::path& source, const fs::path& destination);
[[nodiscard]] std::optional<uintmax_t> getFileSize(const fs::path& path) noexcept;
[[nodiscard]] std::optional<uintmax_t> getDirectorySize(const fs::path& path);
[[nodiscard]] std::optional<std::chrono::system_clock::time_point> getLastWriteTime(const fs::path& path) noexcept;
bool setLastWriteTime(const fs::path& path, std::chrono::system_clock::time_point newTime);

} // namespace DirFunctions
