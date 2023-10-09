#include <iostream>
#include <string>
#include <vector>
#include <algorithm>
#include <iterator>
#include <functional>

class File {
public:
    explicit File(const std::string& filename): _filename{filename}, _isOpen{false} {}
    
    ~File() { close(); }
    
    bool open() {
        if (_isOpen) {
            return true;
        }
        
        // Try to open the file
        _fileStream.open(_filename, std::ios::in | std::ios::out | std::ios::binary);
        if (!_fileStream.good()) {
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
    
    template<typename T>
    bool read(T* value) {
        if (!_isOpen || !value) {
            return false;
        }
        
        // Read the next byte from the file
        unsigned char byte;
        _fileStream >> byte;
        if (!_fileStream.good()) {
            return false;
        }
        
        // Convert the byte to the desired type
        *value = static_cast<T>(byte);
        return true;
    }
    
    template<typename T>
    bool write(const T& value) {
        if (!_isOpen) {
            return false;
        }
        
        // Convert the value to a byte array
        std::array<unsigned char, sizeof(T)> bytes;
        std::memcpy(&bytes[0], &value, sizeof(T));
        
        // Write the byte array to the file
        _fileStream << bytes;
        if (!_fileStream.good()) {
            return false;
        }
        
        return true;
    }
private:
    std::string _filename;
    std::ifstream _fileStream;
    bool _isOpen;
};

int main() {
    File file{"example.txt"};
    if (!file.open()) {
        std::cerr << "Failed to open file\n";
        return 1;
    }
    
    // Read some values from the file
    int x;
    float y;
    double z;
    if (!file.read(&x)) {
        std::cerr << "Failed to read integer from file\n";
        return 1;
    }
    if (!file.read(&y)) {
        std::cerr << "Failed to read floating-point number from file\n";
        return 1;
    }
    if (!file.read(&z)) {
        std::cerr << "Failed to read double precision floating-point number from file\n";
        return 1;
    }
    
    // Print the values
    std::cout << "Integer: " << x << "\n";
    std::cout << "Float: " << y << "\n";
    std::cout << "Double: " << z << "\n";
    
    // Close the file
    file.close();
    return 0;
}
