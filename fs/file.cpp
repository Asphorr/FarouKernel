#include "file.h"
#include <iostream>
#include <stdexcept>

File::File(const std::string& filename) : _filename(filename), _isOpen(false) {}

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
       return true;
   }

   try {
       _fileStream = std::make_unique<std::ofstream>(_filename, std::ios::in | std::ios::out | std::ios::binary);
       if (!_fileStream->good()) {
           throw std::runtime_error("Failed to open file");
       }
   } catch (const std::exception& e) {
       std::cerr << "Error opening file: " << e.what() << '\n';
       return false;
   }

   _isOpen = true;
   return true;
}

void File::close() {
   if (_isOpen) {
       _fileStream->close();
       _isOpen = false;
   }
}

template <WritableType T>
bool File::write(const T& value) {
   if (!_isOpen) {
       return false;
   }

   size_t numBytes = sizeof(T);
   char buffer[numBytes];

   try {
       _fileStream->write(buffer, numBytes);
       if (!_fileStream->good()) {
           throw std::runtime_error("Failed to write to file");
       }
   } catch (const std::exception& e) {
       std::cerr << "Error writing to file: " << e.what() << '\n';
       return false;
   }

   return true;
}

template <ReadableType T>
bool File::read(T& value) {
   if (!_isOpen) {
       return false;
   }

   size_t numBytes = sizeof(T);
   char buffer[numBytes];

   try {
       _fileStream->read(buffer, numBytes);
       if (!_fileStream->good()) {
           throw std::runtime_error("Failed to read from file");
       }
   } catch (const std::exception& e) {
       std::cerr << "Error reading from file: " << e.what() << '\n';
       return false;
   }

   memcpy(&value, buffer, numBytes);
   return true;
}
