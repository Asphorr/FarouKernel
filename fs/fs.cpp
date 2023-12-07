#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <filesystem>
#include <cstdlib>

class FileSystem {
public:
   FileSystem() : current_("."), root_(".") {}

   void cd(const std::string& path) {
       current_ = path;
   }

   void pwd() {
       std::cout << current_ << '\n';
   }

   void mkdir(const std::string& path) {
       std::filesystem::create_directory(path);
   }

   void rmdir(const std::string& path) {
       std::filesystem::remove_all(path);
   }

   void touch(const std::string& filename) {
       std::ofstream out(filename);
       out.close();
   }

   void cat(const std::string& filename) {
       std::ifstream in(filename);
       std::string line;
       while (std::getline(in, line)) {
           std::cout << line << '\n';
       }
       in.close();
   }

   void echo(const std::string& text) {
       std::ofstream out(text);
       out.close();
   }

   std::vector<std::string> ls(const std::string& path) {
       std::vector<std::string> result;
       for (const auto& entry : std::filesystem::directory_iterator(path)) {
           result.push_back(entry.path().filename().string());
       }
       return result;
   }

private:
   std::string current_;
   std::string root_;
};

int main() {
   FileSystem fs;

   // Create some directories
   fs.mkdir("mydir");
   fs.mkdir("mydir/subdir1");
   fs.mkdir("mydir/subdir2");

   // Print the contents of the root directory
   fs.pwd();
   for (auto& dir : fs.ls("/")) {
       std::cout << "\t" << dir << '\n';
   }

   // Change into the subdirectory
   fs.cd("mydir/subdir1");

   // Print the contents of the subdirectory
   fs.pwd();
   for (auto& dir : fs.ls(".")) {
       std::cout << "\t" << dir << '\n';
   }

   // Go back up one level
   fs.cd("..");

   // Delete the subdirectories
   fs.rmdir("mydir/subdir1");
   fs.rmdir("mydir/subdir2");

   // Print the contents of the root directory again
   fs.pwd();
   for (auto& dir : fs.ls("/")) {
       std::cout << "\t" << dir << '\n';
   }

   return 0;
}
