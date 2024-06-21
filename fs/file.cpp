#include "File.h"

File::File(const std::string& filename, Mode mode) : _filename(filename) {
    open(mode);
}

File::~File() {
    if (_isOpen) {
        close();
    }
}

bool File::isOpen() const {
    return _isOpen;
}

void File::open(Mode mode) {
    if (_isOpen) {
        return;
    }

    try {
        switch (mode) {
            case Mode::Read:
                openForRead();
                break;
            case Mode::Write:
                openForWrite();
                break;
            case Mode::Append:
                openForAppend();
                break;
            default:
                _errorMessage = "Invalid file mode.";
                throw std::invalid_argument(_errorMessage);
        }
        _isOpen = true;
    } catch (const std::exception& ex) {
        _errorMessage = ex.what();
    }
}

void File::openForRead() {
    _inputStream = std::make_unique<std::ifstream>(_filename);
    if (!_inputStream->is_open()) {
        _errorMessage = "Failed to open file for reading: " + _filename;
        _inputStream.reset();
        throw std::runtime_error(_errorMessage);
    }
}

void File::openForWrite() {
    _outputStream = std::make_unique<std::ofstream>(_filename, std::ofstream::trunc);
    if (!_outputStream->is_open()) {
        _errorMessage = "Failed to open file for writing: " + _filename;
        _outputStream.reset();
        throw std::runtime_error(_errorMessage);
    }
}

void File::openForAppend() {
    _outputStream = std::make_unique<std::ofstream>(_filename, std::ofstream::app);
    if (!_outputStream->is_open()) {
        _errorMessage = "Failed to open file for appending: " + _filename;
        _outputStream.reset();
        throw std::runtime_error(_errorMessage);
    }
}

void File::close() {
    if (_inputStream) {
        _inputStream->close();
        _inputStream.reset();
    }

    if (_outputStream) {
        _outputStream->close();
        _outputStream.reset();
    }

    _isOpen = false;
}

std::string File::getErrorMessage() const {
    return _errorMessage;
}

bool File::writeLine(const std::string& line) {
    if (!_isOpen || !_outputStream || !_outputStream->good()) {
        _errorMessage = "Write operation failed: Invalid output stream.";
        return false;
    }

    *_outputStream << line << std::endl;
    if (!_outputStream->good()) {
        _errorMessage = "Write operation failed: Stream error.";
        return false;
    }

    return true;
}

bool File::readLine(std::string& line) {
    if (!_isOpen || !_inputStream || !_inputStream->good()) {
        _errorMessage = "Read operation failed: Invalid input stream.";
        return false;
    }

    std::getline(*_inputStream, line);
    if (_inputStream->fail()) {
        _errorMessage = "Read operation failed: Stream error.";
        return false;
    }

    return true;
}
