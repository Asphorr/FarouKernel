#include <iostream>
#include <fstream>
#include <filesystem>
#include <string>
#include <memory>

class FileReader {
public:
    explicit FileReader(const std::string& filename);
    virtual ~FileReader();

    bool open();
    bool close();
    bool read(std::string& content);

private:
    std::string filename;
    std::unique_ptr<std::ifstream> fileStream;
};

class FileWriter {
public:
    explicit FileWriter(const std::string& filename);
    virtual ~FileWriter();

    bool open();
    bool close();
    bool write(const std::string& content);

private:
    std::string filename;
    std::unique_ptr<std::ofstream> fileStream;
};

int main() {
    std::string filename = "input.txt";

    FileReader fileReader(filename);
    if (fileReader.open()) {
        std::string content;
        fileReader.read(content);
        std::cout << "Content: " << content << std::endl;
        fileReader.close();
    }

    FileWriter fileWriter(filename);
    if (fileWriter.open()) {
        fileWriter.write("New content");
        fileWriter.close();
    }

    return 0;
}
