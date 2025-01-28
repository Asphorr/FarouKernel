#ifndef DIR_FUNCTIONS_H
#define DIR_FUNCTIONS_H

#include <string>
#include <vector>
#include <filesystem>
#include <fstream>
#include <stdexcept>

namespace fs = std::filesystem;

namespace DirFunctions {

void CreateDirectoryAndWriteToFile(const std::string& dirPath, 
                                   const std::vector<std::string>& data, 
                                   fs::perms perms = fs::perms::all);

void DeleteDirectory(const std::string& dirPath, bool deleteNonEmpty = false);

void CopyDirectory(const std::string& sourcePath, 
                   const std::string& destinationPath, 
                   bool copySelf = false);

std::vector<std::string> GetAllFilesInDir(const std::string& dirPath, 
                                          bool fullPath = false);

bool IsDirectoryEmpty(const std::string& dirPath);

bool DoesFileExist(const std::string& filePath);

std::string ReadFromFile(const std::string& filePath);

} // namespace DirFunctions

#endif // DIR_FUNCTIONS_H
