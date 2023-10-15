#include <fstream>
#include <type_traits>

template <typename T>
concept WritableType = requires(T t) {
    { t.write(std::declval<char[]>(), std::declval<size_t>())} -> std::same_as<void>;
};

template <typename T>
concept ReadableType = requires(T t) {
    { t.read(std::declval<char[]>(), std::declval<size_t>())} -> std::same_as<void>;
};

class File {
public:
    explicit File(const std::string& filename) : _filename(filename), _isOpen(false) {}

    virtual ~File() { close(); }

    bool open() {
        if (_isOpen) {
            return true;
        }

        try {
            _fileStream.open(_filename, std::ios::in | std::ios::out | std::ios::binary);
            if (!_fileStream.good()) {
                throw std::runtime_error("Failed to open file");
            }
        } catch (const std::exception& e) {
            std::cerr << "Error opening file: " << e.what() << '\n';
            return false;
        }

        _isOpen = true;
        return true;
    }

    void close() {
        if (_isOpen) {
            _fileStream.close();
            _isOpen = false;
        }
    }

    template <ReadableType T>
    bool read(T& value) {
        if (!_isOpen) {
            return false;
        }

        size_t numBytes = sizeof(T);
        char buffer[numBytes];

        try {
            _fileStream.read(buffer, numBytes);
            if (!_fileStream.good()) {
                throw std::runtime_error("Failed to read from file");
            }
        } catch (const std::exception& e) {
            std::cerr << "Error reading from file: " << e.what() << '\n';
            return false;
        }

        memcpy(&value, buffer, numBytes);
        return true;
    }

    template <WritableType T>
    bool write(const T& value) {
        if (!_isOpen) {
            return false;
        }

        size_t numBytes = sizeof(T);
        char buffer[numBytes];

        try {
            _fileStream.write(buffer, numBytes);
            if (!_fileStream.good()) {
                throw std::runtime_error("Failed to write to file");
            }
        } catch (const std::exception& e) {
            std::cerr << "Error writing to file: " << e.what() << '\n';
            return false;
        }

        return true;
    }

private:
    std::string _filename;
    std::ifstream _fileStream;
    bool _isOpen;
};
