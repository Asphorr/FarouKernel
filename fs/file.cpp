// File.h
#ifndef FILE_H
#define FILE_H

#include <fstream>
#include <memory>
#include <string>

class File {
public:
    explicit File(const std::string& filename);
    ~File();

    bool isOpen() const;
    bool open();
    void close();

    template<typename T>
    bool write(const T& value);

    template<typename T>
    bool read(T& value);

private:
    std::string _filename;
    std::unique_ptr<std::fstream> _fileStream;
    bool _isOpen = false;
};

#include "File.hpp" // Include template implementations

#endif // FILE_H

// File.cpp
#include "File.h"

File::File(const std::string& filename) : _filename(filename) {}

File::~File() {
    if (_isOpen) {
        close();
    }
}

bool File::isOpen() const {
    return _isOpen;
}

bool File::open() {
    if (_isOpen) {
        return false;
    }
    _fileStream = std::make_unique<std::fstream>(_filename, std::fstream::in | std::fstream::out | std::fstream::app);
    if (!_fileStream->is_open()) {
        _fileStream.reset();
        return false;
    }
    _isOpen = true;
    return true;
}

void File::close() {
    if (_isOpen) {
        _fileStream->close();
        _fileStream.reset();
        _isOpen = false;
    }
}

// File.hpp
template<typename T>
bool File::write(const T& value) {
    if (!_isOpen || !_fileStream->good()) {
        return false;
    }
    *_fileStream << value;
    return _fileStream->good();
}

template<typename T>
bool File::read(T& value) {
    if (!_isOpen || !_fileStream->good()) {
        return false;
    }
    *_fileStream >> value;
    return _fileStream->good();
}