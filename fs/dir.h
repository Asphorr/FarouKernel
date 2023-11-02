#ifndef DIR_H
#define DIR_H

#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <string>
#include <sys/types.h>
#include <unistd.h>

// Define an alias for the filesystem library
namespace fs = std::filesystem;

// Define a class to represent a directory
class Directory {
public:
  // Default constructor
  Directory() : m_path(), m_flags(O_RDONLY | O_CLOEXEC), m_handle(-1) {}

  // Constructor taking a file path and optional flags
  explicit Directory(fs::path path, int flags = O_RDONLY | O_CLOEXEC)
      : m_path(std::move(path)), m_flags(flags), m_handle(-1) {}

  // Move constructor and assignment operator
  Directory(Directory&& other) noexcept
      : m_path(other.m_path), m_flags(other.m_flags), m_handle(other.m_handle) {
    other.m_handle = -1;
  }

  Directory& operator=(Directory&& other) noexcept {
    swap(*this, other);
    return *this;
  }

  friend void swap(Directory& lhs, Directory& rhs) noexcept {
    using std::swap;
    swap(lhs.m_path, rhs.m_path);
    swap(lhs.m_flags, rhs.m_flags);
    swap(lhs.m_handle, rhs.m_handle);
  }

  // Destructor closes the directory handle if necessary
  ~Directory() noexcept {
    if (m_handle != -1) {
      close(m_handle);
    }
  }

  // Getters for the directory path and flags
  const fs::path& path() const { return m_path; }
  int flags() const { return m_flags; }

  // Returns true if the directory is currently open
  bool isOpen() const { return m_handle != -1; }

  // Opens the directory
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

  // Closes the directory
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

// Function to get the full path of a file within a directory
fs::path getFilePath(const Directory& directory, const char* filename) {
  return directory.path().append(filename).make_preferred();
}

// Function to check if one directory is a subdirectory of another
bool isSubdirectoryOf(const Directory& parent, const Directory& child) {
  auto parentPath = parent.path().lexically_normal();
  auto childPath = child.path().lexically_normal();
  return !childPath.empty() && childPath.starts_with(parentPath);
}

// Function to print the contents of a directory
void listFilesInDirectory(const Directory& directory) {
  try {
    // Open the directory
    directory.open();

    // Iterate over the entries in the directory
    for (auto& entry : directory) {
      // Print the name of the current entry
      std::cout << entry.d_name << '\n';
    }

    // Close the directory
    directory.close();
  } catch (const std::exception& e) {
    std::cerr << "Error listing files in directory: " << e.what() << '\n';
  }
}

#endif /* DIR_H */
