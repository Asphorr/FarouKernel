#include "File.h"

File::File(const std::string& filename) :
    _filename(filename),
    _fileStream(nullptr),
    _isOpen(false) {
}

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
    _fileStream = std::make_unique<std::ofstream>(_filename);
    if (!_fileStream->is_open()) {
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

template <WritableType T>
bool File::write(const T& value) {
    if (!_isOpen) {
        return false;
    }
    *_fileStream << value;
    return true;
}

template <ReadableType T>
bool File::read(T& value) {
    if (!_isOpen) {
        return false;
    }
    *_fileStream >> value;
    return true;
}

// Explicitly instantiate the templates for int and float
template bool File::write(const int& value);
template bool File::write(const float& value);
template bool File::read(int& value);
template bool File::read(float& value);
