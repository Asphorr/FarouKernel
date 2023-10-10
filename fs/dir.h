#include <filesystem>
#include <iostream>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <utility>

// Define an alias for the filesystem library
namespace fs = std::filesystem;

// Define a class to represent a directory
class Directory {
 public:
  // Constructor takes a file path and optionally a set of flags
  explicit Directory(fs::path path, int flags = O_RDONLY | O_CLOEXEC)
      : m_path(std::move(path)), m_flags(flags), m_handle(-1) {}

  // Destructor closes the directory handle if necessary
  ~Directory() noexcept {
    if (m_handle != -1) {
      close(m_handle);
    }
  }

  // Getter methods for the directory path and flags
  const fs::path& path() const { return m_path; }
  int flags() const { return m_flags; }

  // Method to open the directory
  void open() {
    // Check if the directory is already open
    if (m_handle != -1) {
      throw std::runtime_error("Directory already open");
    }

    // Open the directory using the provided flags
    m_handle = ::openat(AT_FDCWD, m_path.c_str(), m_flags);
    if (m_handle == -1) {
      throw std::system_error(errno, std::generic_category());
    }
  }

  // Method to close the directory
  void close() {
    // Check if the directory is open
    if (m_handle == -1) {
      throw std::runtime_error("Directory not open");
    }

    // Close the directory
    if (::close(m_handle) == -1) {
      throw std::system_error(errno, std::generic_category());
    }

    // Reset the handle to indicate that the directory is closed
    m_handle = -1;
  }

 private:
  // Member variables for the directory path, flags, and handle
  fs::path m_path;
  int m_flags;
  int m_handle;
};

// Function to print the contents of a directory
void listFilesInDirectory(const Directory& directory) {
  // Open the directory
  directory.open();

  // Read each entry from the directory until EOF is reached
  struct dirent* entry;
  while ((entry = readdir(directory.getHandle())) != nullptr) {
    // Print the name of the current entry
    std::cout << entry->d_name << '\n';
  }

  // Close the directory
  directory.close();
}

// Main function
int main() {
  try {
    // Create a new directory instance
    Directory directory(".", O_RDONLY | O_CLOEXEC);

    // List the files in the directory
    listFilesInDirectory(directory);
  } catch (const std::exception& e) {
    std::cerr << "Error: " << e.what() << '\n';
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}
