#include <string>
#include <vector>
#include <functional>
#include <memory>
#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <chrono>

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
    std::chrono::system_clock::time_point getLastModifiedTime() const;

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

File::~File() = default;

File::File(File&&) noexcept = default;
File& File::operator=(File&&) noexcept = default;

bool File::isOpen() const noexcept {
    return pImpl->stream.is_open();
}

void File::open(Mode mode) {
    if (isOpen()) close();

    std::ios_base::openmode flags;
    switch (mode) {
        case Mode::Read: flags = std::ios_base::in; break;
        case Mode::Write: flags = std::ios_base::out | std::ios_base::trunc; break;
        case Mode::Append: flags = std::ios_base::out | std::ios_base::app; break;
        case Mode::ReadWrite: flags = std::ios_base::in | std::ios_base::out; break;
        default: throw Exception("Invalid file mode");
    }

    pImpl->stream.open(pImpl->filePath, flags);
    if (!pImpl->stream) {
        throw Exception("Failed to open file: " + pImpl->filePath.string());
    }
    pImpl->mode = mode;
}

void File::close() noexcept {
    pImpl->stream.close();
}

std::string File::readAll() {
    if (pImpl->mode == Mode::Write || pImpl->mode == Mode::Append) {
        throw Exception("File not opened for reading");
    }

    pImpl->stream.seekg(0, std::ios::beg);
    return std::string(std::istreambuf_iterator<char>(pImpl->stream), std::istreambuf_iterator<char>());
}

std::vector<std::string> File::readLines() {
    std::vector<std::string> lines;
    std::string line;
    while (std::getline(pImpl->stream, line)) {
        lines.push_back(std::move(line));
    }
    return lines;
}

std::string File::readLine() {
    std::string line;
    std::getline(pImpl->stream, line);
    return line;
}

std::vector<char> File::readBytes(size_t count) {
    std::vector<char> buffer(count);
    pImpl->stream.read(buffer.data(), count);
    buffer.resize(pImpl->stream.gcount());
    return buffer;
}

void File::write(const std::string& data) {
    pImpl->stream << data;
    if (!pImpl->stream) throw Exception("Write operation failed");
}

void File::writeLine(const std::string& line) {
    pImpl->stream << line << '\n';
    if (!pImpl->stream) throw Exception("Write operation failed");
}

void File::writeLines(const std::vector<std::string>& lines) {
    for (const auto& line : lines) {
        writeLine(line);
    }
}

void File::writeBytes(const std::vector<char>& bytes) {
    pImpl->stream.write(bytes.data(), bytes.size());
    if (!pImpl->stream) throw Exception("Write operation failed");
}

void File::seek(std::streamoff offset, SeekOrigin origin) {
    std::ios_base::seekdir dir;
    switch (origin) {
        case SeekOrigin::Begin: dir = std::ios_base::beg; break;
        case SeekOrigin::Current: dir = std::ios_base::cur; break;
        case SeekOrigin::End: dir = std::ios_base::end; break;
    }
    pImpl->stream.seekg(offset, dir);
    pImpl->stream.seekp(offset, dir);
}

std::streampos File::tell() const {
    return pImpl->stream.tellg();
}

void File::flush() {
    pImpl->stream.flush();
}

std::filesystem::path File::getPath() const {
    return pImpl->filePath;
}

std::uintmax_t File::getSize() const {
    return std::filesystem::file_size(pImpl->filePath);
}

std::chrono::system_clock::time_point File::getLastModifiedTime() const {
    return std::filesystem::last_write_time(pImpl->filePath);
}

void File::withFile(const std::string& path, Mode mode, FileCallback callback) {
    File file(path, mode);
    callback(file);
}

bool File::exists(const std::string& path) {
    return std::filesystem::exists(path);
}

void File::copy(const std::string& from, const std::string& to, bool overwrite) {
    std::filesystem::copy(from, to, overwrite ? std::filesystem::copy_options::overwrite_existing : std::filesystem::copy_options::none);
}

void File::move(const std::string& from, const std::string& to) {
    std::filesystem::rename(from, to);
}

void File::remove(const std::string& path) {
    std::filesystem::remove(path);
}
