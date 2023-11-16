#ifndef DIR_FUNCTIONS_H
#define DIR_FUNCTIONS_H

#include <string>
#include <vector>
#include <filesystem>

namespace DirFunctions {

void createDirectoryAndWriteToFile(const std::filesystem::path& dirPath, const std::vector<std::string>& data);
void readFromFileAndDeleteIt(const std::filesystem::path& filePath);
void deleteEmptyDirectory(const std::filesystem::path& dirPath);
bool hasFilesInDir(const std::filesystem::path& dirPath);
void copyDirectory(const std::filesystem::path& sourcePath, const std::filesystem::path& destinationPath);
std::vector<std::filesystem::path> getAllFilesInDir(const std::filesystem::path& dirPath);
std::vector<std::filesystem::path> getAllDirectoriesInDir(const std::filesystem::path& dirPath);

} // namespace DirFunctions

#endif // DIR_FUNCTIONS_H
