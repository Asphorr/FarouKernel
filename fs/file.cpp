#include <string>
#include <vector>
#include <functional>
#include <memory>
#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <chrono>
#include <iterator>
#include <sstream>

class File {
public:
    enum class Mode { Read, Write, Append, ReadWrite };
    enum class SeekOrigin { Begin, Current, End };

    using FileCallback = std::function<void(File&)>;

    File(std::string path, Mode mode = Mode::Read);
    ~File();

    File(const File&) = delete;
    File& operator=(const File&) = delete;
    File(File&&) noexcept;
    File& operator=(File&&) noexcept;

    bool isOpen() const noexcept;
    void open(Mode mode);
    void close() noexcept;

    std::string readAll();
    std::vector<std::string> readLines();
    std::string readLine();
    std::vector<char> readBytes(size_t count);

    void write(const std::string& data);
    void writeLine(const std::string& line);
    void writeLines(const std::vector<std::string>& lines);
    void writeBytes(const std::vector<char>& bytes);

    void seek(std::streamoff offset, SeekOrigin origin = SeekOrigin::Begin);
    std::streampos tell() const;

    void flush();

    std::filesystem::path getPath() const;
    std::uintmax_t getSize() const;
    std::filesystem::file_time_type getLastModifiedTime() const;

    static void withFile(const std::string& path, Mode mode, FileCallback callback);
    static bool exists(const std::string& path);
    static void copy(const std::string& from, const std::string& to, bool overwrite = false);
    static void move(const std::string& from, const std::string& to);
    static void remove(const std::string& path);

    class Exception : public std::runtime_error {
    public:
        explicit Exception(const std::string& message) : std::runtime_error(message) {}
    };

private:
    class Impl;
    std::unique_ptr<Impl> pImpl;
};

// Implementation

class File::Impl {
public:
    std::fstream stream;
    std::filesystem::path filePath;
    Mode mode;

    Impl(const std::string& path, Mode m) : filePath(path), mode(m) {}
};

File::File(std::string path, Mode mode) : pImpl(std::make_unique<Impl>(std::move(path), mode)) {
    open(mode);
}

File::~File() {
    if (isOpen()) {
        close();
    }
}

File::File(File&& other) noexcept : pImpl(std::move(other.pImpl)) {}

File& File::operator=(File&& other) noexcept {
    if (this != &other) {
        if (isOpen()) {
            close();
        }
        pImpl = std::move(other.pImpl);
    }
    return *this;
}

bool File::isOpen() const noexcept {
    return pImpl && pImpl->stream.is_open();
}

void File::open(Mode mode) {
    if (!pImpl) {
        throw Exception("File object not properly initialized");
    }
    
    if (isOpen()) {
        close();
    }

    std::ios_base::openmode flags = std::ios_base::binary;
    switch (mode) {
        case Mode::Read: 
            flags |= std::ios_base::in; 
            break;
        case Mode::Write: 
            flags |= std::ios_base::out | std::ios_base::trunc; 
            break;
        case Mode::Append: 
            flags |= std::ios_base::out | std::ios_base::app; 
            break;
        case Mode::ReadWrite: 
            flags |= std::ios_base::in | std::ios_base::out; 
            break;
    }

    // Create directory if it doesn't exist (for write modes)
    if (mode != Mode::Read) {
        auto parent = pImpl->filePath.parent_path();
        if (!parent.empty() && !std::filesystem::exists(parent)) {
            std::filesystem::create_directories(parent);
        }
    }

    pImpl->stream.open(pImpl->filePath, flags);
    if (!pImpl->stream.is_open() || !pImpl->stream.good()) {
        throw Exception("Failed to open file: " + pImpl->filePath.string() + 
                       " (Error: " + std::strerror(errno) + ")");
    }
    pImpl->mode = mode;
}

void File::close() noexcept {
    if (pImpl && pImpl->stream.is_open()) {
        pImpl->stream.flush();
        pImpl->stream.close();
    }
}

std::string File::readAll() {
    if (!pImpl) {
        throw Exception("File object not properly initialized");
    }
    
    if (!isOpen()) {
        throw Exception("File is not open");
    }
    
    if (pImpl->mode == Mode::Write || pImpl->mode == Mode::Append) {
        throw Exception("File not opened for reading");
    }

    // Save current position
    auto currentPos = pImpl->stream.tellg();
    
    // Read entire file
    pImpl->stream.seekg(0, std::ios::end);
    auto fileSize = pImpl->stream.tellg();
    pImpl->stream.seekg(0, std::ios::beg);
    
    std::string content;
    content.reserve(static_cast<size_t>(fileSize));
    content.assign(std::istreambuf_iterator<char>(pImpl->stream), 
                   std::istreambuf_iterator<char>());
    
    // Clear any error flags and restore position
    pImpl->stream.clear();
    pImpl->stream.seekg(currentPos);
    
    return content;
}

std::vector<std::string> File::readLines() {
    if (!pImpl) {
        throw Exception("File object not properly initialized");
    }
    
    if (!isOpen()) {
        throw Exception("File is not open");
    }
    
    if (pImpl->mode == Mode::Write || pImpl->mode == Mode::Append) {
        throw Exception("File not opened for reading");
    }

    std::vector<std::string> lines;
    std::string line;
    
    // Save current position
    auto currentPos = pImpl->stream.tellg();
    pImpl->stream.seekg(0, std::ios::beg);
    
    while (std::getline(pImpl->stream, line)) {
        // Handle different line endings (CRLF, LF)
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }
        lines.push_back(std::move(line));
    }
    
    // Clear EOF flag and restore position
    pImpl->stream.clear();
    pImpl->stream.seekg(currentPos);
    
    return lines;
}

std::string File::readLine() {
    if (!pImpl) {
        throw Exception("File object not properly initialized");
    }
    
    if (!isOpen()) {
        throw Exception("File is not open");
    }
    
    if (pImpl->mode == Mode::Write || pImpl->mode == Mode::Append) {
        throw Exception("File not opened for reading");
    }

    std::string line;
    if (std::getline(pImpl->stream, line)) {
        // Handle different line endings (CRLF, LF)
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }
    }
    return line;
}

std::vector<char> File::readBytes(size_t count) {
    if (!pImpl) {
        throw Exception("File object not properly initialized");
    }
    
    if (!isOpen()) {
        throw Exception("File is not open");
    }
    
    if (pImpl->mode == Mode::Write || pImpl->mode == Mode::Append) {
        throw Exception("File not opened for reading");
    }

    std::vector<char> buffer(count);
    pImpl->stream.read(buffer.data(), static_cast<std::streamsize>(count));
    
    auto bytesRead = pImpl->stream.gcount();
    if (bytesRead < static_cast<std::streamsize>(count)) {
        buffer.resize(static_cast<size_t>(bytesRead));
    }
    
    return buffer;
}

void File::write(const std::string& data) {
    if (!pImpl) {
        throw Exception("File object not properly initialized");
    }
    
    if (!isOpen()) {
        throw Exception("File is not open");
    }
    
    if (pImpl->mode == Mode::Read) {
        throw Exception("File not opened for writing");
    }

    pImpl->stream.write(data.c_str(), static_cast<std::streamsize>(data.size()));
    if (!pImpl->stream.good()) {
        throw Exception("Write operation failed");
    }
}

void File::writeLine(const std::string& line) {
    if (!pImpl) {
        throw Exception("File object not properly initialized");
    }
    
    if (!isOpen()) {
        throw Exception("File is not open");
    }
    
    if (pImpl->mode == Mode::Read) {
        throw Exception("File not opened for writing");
    }

    pImpl->stream.write(line.c_str(), static_cast<std::streamsize>(line.size()));
    pImpl->stream.put('\n');
    
    if (!pImpl->stream.good()) {
        throw Exception("Write operation failed");
    }
}

void File::writeLines(const std::vector<std::string>& lines) {
    if (!pImpl) {
        throw Exception("File object not properly initialized");
    }
    
    if (!isOpen()) {
        throw Exception("File is not open");
    }
    
    if (pImpl->mode == Mode::Read) {
        throw Exception("File not opened for writing");
    }

    for (const auto& line : lines) {
        pImpl->stream.write(line.c_str(), static_cast<std::streamsize>(line.size()));
        pImpl->stream.put('\n');
        
        if (!pImpl->stream.good()) {
            throw Exception("Write operation failed at line: " + line);
        }
    }
}

void File::writeBytes(const std::vector<char>& bytes) {
    if (!pImpl) {
        throw Exception("File object not properly initialized");
    }
    
    if (!isOpen()) {
        throw Exception("File is not open");
    }
    
    if (pImpl->mode == Mode::Read) {
        throw Exception("File not opened for writing");
    }

    if (!bytes.empty()) {
        pImpl->stream.write(bytes.data(), static_cast<std::streamsize>(bytes.size()));
        if (!pImpl->stream.good()) {
            throw Exception("Write operation failed");
        }
    }
}

void File::seek(std::streamoff offset, SeekOrigin origin) {
    if (!pImpl) {
        throw Exception("File object not properly initialized");
    }
    
    if (!isOpen()) {
        throw Exception("File is not open");
    }

    std::ios_base::seekdir dir;
    switch (origin) {
        case SeekOrigin::Begin: 
            dir = std::ios_base::beg; 
            break;
        case SeekOrigin::Current: 
            dir = std::ios_base::cur; 
            break;
        case SeekOrigin::End: 
            dir = std::ios_base::end; 
            break;
        default:
            throw Exception("Invalid seek origin");
    }
    
    // Clear any error flags before seeking
    pImpl->stream.clear();
    
    if (pImpl->mode == Mode::Read || pImpl->mode == Mode::ReadWrite) {
        pImpl->stream.seekg(offset, dir);
        if (!pImpl->stream.good() && !pImpl->stream.eof()) {
            throw Exception("Seek operation failed for reading");
        }
    }
    
    if (pImpl->mode == Mode::Write || pImpl->mode == Mode::Append || pImpl->mode == Mode::ReadWrite) {
        pImpl->stream.seekp(offset, dir);
        if (!pImpl->stream.good()) {
            throw Exception("Seek operation failed for writing");
        }
    }
}

std::streampos File::tell() const {
    if (!pImpl) {
        throw Exception("File object not properly initialized");
    }
    
    if (!isOpen()) {
        throw Exception("File is not open");
    }
    
    std::streampos pos = -1;
    
    if (pImpl->mode == Mode::Read || pImpl->mode == Mode::ReadWrite) {
        pos = pImpl->stream.tellg();
    } else if (pImpl->mode == Mode::Write || pImpl->mode == Mode::Append) {
        pos = pImpl->stream.tellp();
    }
    
    if (pos == std::streampos(-1)) {
        throw Exception("Tell operation failed");
    }
    
    return pos;
}

void File::flush() {
    if (!pImpl) {
        throw Exception("File object not properly initialized");
    }
    
    if (!isOpen()) {
        throw Exception("File is not open");
    }
    
    pImpl->stream.flush();
    if (!pImpl->stream.good()) {
        throw Exception("Flush operation failed");
    }
}

std::filesystem::path File::getPath() const {
    if (!pImpl) {
        throw Exception("File object not properly initialized");
    }
    return pImpl->filePath;
}

std::uintmax_t File::getSize() const {
    if (!pImpl) {
        throw Exception("File object not properly initialized");
    }
    
    try {
        return std::filesystem::file_size(pImpl->filePath);
    } catch (const std::filesystem::filesystem_error& e) {
        throw Exception("Failed to get file size: " + std::string(e.what()));
    }
}

std::filesystem::file_time_type File::getLastModifiedTime() const {
    if (!pImpl) {
        throw Exception("File object not properly initialized");
    }
    
    try {
        return std::filesystem::last_write_time(pImpl->filePath);
    } catch (const std::filesystem::filesystem_error& e) {
        throw Exception("Failed to get last modified time: " + std::string(e.what()));
    }
}

void File::withFile(const std::string& path, Mode mode, FileCallback callback) {
    File file(path, mode);
    try {
        callback(file);
    } catch (...) {
        // Ensure file is closed even if callback throws
        file.close();
        throw;
    }
}

bool File::exists(const std::string& path) {
    try {
        return std::filesystem::exists(path);
    } catch (const std::filesystem::filesystem_error&) {
        return false;
    }
}

void File::copy(const std::string& from, const std::string& to, bool overwrite) {
    try {
        auto options = overwrite ? std::filesystem::copy_options::overwrite_existing 
                                 : std::filesystem::copy_options::none;
        std::filesystem::copy_file(from, to, options);
    } catch (const std::filesystem::filesystem_error& e) {
        throw Exception("Failed to copy file: " + std::string(e.what()));
    }
}

void File::move(const std::string& from, const std::string& to) {
    try {
        std::filesystem::rename(from, to);
    } catch (const std::filesystem::filesystem_error& e) {
        throw Exception("Failed to move file: " + std::string(e.what()));
    }
}

void File::remove(const std::string& path) {
    try {
        if (!std::filesystem::exists(path)) {
            throw Exception("File does not exist: " + path);
        }
        std::filesystem::remove(path);
    } catch (const std::filesystem::filesystem_error& e) {
        throw Exception("Failed to remove file: " + std::string(e.what()));
    }
}

