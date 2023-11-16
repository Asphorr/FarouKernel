#ifndef DIR_FUNCTIONS_H
#define DIR_FUNCTIONS_H

#include <string>
#include <vector>
#include <filesystem>

void createDirectoryAndWriteToFile(const std::filesystem::path& dirPath, const std::vector<std::string>& data);
void readFromFileAndDeleteIt(const std::filesystem::path& filePath);
void deleteEmptyDirectory(const std::filesystem::path& dirPath);
bool hasFilesInDir(const std::filesystem::path& dirPath);

#endif
