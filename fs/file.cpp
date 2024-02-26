#include <fstream>
#include <iostream>
#include <stdexcept>
#include <system_error>

class File {
public:
    using Mode = std::ios_base::openmode;

    explicit File(const std::string& filename) : _filename(filename), _fileStream(), _isOpen(false) {}

    ~File() {
        if (_isOpen) {
            close();
        }
    }

    bool isOpen() const {
        return _isOpen;
    }

    bool open(Mode mode = Mode(std::ios::in | std::ios::out | std::ios::binary)) {
        if (_isOpen) {
            return true;
        }

        try {
            _fileStream = std::make_unique<std::fstream>(_filename, mode);
            if (!_fileStream->good()) {
                throw std::system_error(errno, std::generic_category(), "Failed to open file");
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
            _fileStream->close();
            _isOpen = false;
        }
    }

    template <typename T>
    bool write(const T& value) {
        if (!_isOpen) {
            return false;
        }

        try {
            _fileStream->write(reinterpret_cast<const char*>(&value), sizeof(T));
            if (!_fileStream->good()) {
                throw std::system_error(errno, std::generic_category(), "Failed to write to file");
            }
        } catch (const std::exception& e) {
            std::cerr << "Error writing to file: " << e.what() << '\n';
            return false;
        }

        return true;
    }

    template <typename T>
    bool read(T& value) {
        if (!_isOpen) {
            return false;
        }

        try {
            _fileStream->read(reinterpret_cast<char*>(&value), sizeof(T));
            if (!_fileStream->good()) {
                throw std::system_error(errno, std::generic_category(), "Failed to read from file");
            }
        } catch (const std::exception& e) {
            std::cerr << "Error reading from file: " << e.what() << '\n';
            return false;
        }

        return true;
    }

private:
    std::string _filename;
    std::unique_ptr<std::fstream> _fileStream;
    bool _isOpen;
};
