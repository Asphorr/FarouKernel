#pragma once

#include <stdint.h>
#include <string.h>
#include <sys/types.h>

typedef struct dirent {
    ino_t d_ino;       // Inode number
    off_t d_off;       // Offset to the start of the file
    uint16_t d_reclen; // Length of this directory entry
    uint8_t d_type;    // Type of file (see below)
    char d_name[256]; // Name of the file
} dirent;

// Enum for file types
enum FileType {
    REGULAR_FILE = 1,
    DIRECTORY = 2,
    SYMBOLIC_LINK = 3,
    BLOCK_DEVICE = 4,
    CHARACTER_DEVICE = 5,
    FIFO = 6,
    SOCKET = 7
};

// Function pointers for directory operations
typedef void (*DirInitFunc)(void);
typedef void (*DirExitFunc)(void);
typedef int (*DirOpenDirFunc)(const char* path, int flags);
typedef int (*DirCloseDirFunc)(int dirp);
typedef struct dirent* (*DirReadDirFunc)(int dirp);
typedef int (*DirSeekDirFunc)(int dirp, off_t offset);
typedef int (*DirTellDirFunc)(int dirp);
typedef int (*DirMkDirFunc)(const char* path, mode_t mode);
typedef int (*DirRmDirFunc)(const char* path);
typedef int (*DirRenameFunc)(const char* oldpath, const char* newpath);

// Struct for storing directory operation function pointers
typedef struct {
    DirInitFunc init;
    DirExitFunc exit;
    DirOpenDirFunc opendir;
    DirCloseDirFunc closedir;
    DirReadDirFunc readdir;
    DirSeekDirFunc seekdir;
    DirTellDirFunc telldir;
    DirMkDirFunc mkdir;
    DirRmDirFunc rmdir;
    DirRenameFunc rename;
} DirOps;

// Global directory operations object
static DirOps g_dir_ops;

// Initializes the directory system
inline static void dir_init() {
    if (!g_dir_ops.init) {
        g_dir_ops.init = &dir_init;
    }
    g_dir_ops.init();
}

// Cleans up the directory system
inline static void dir_exit() {
    if (!g_dir_ops.exit) {
        g_dir_ops.exit = &dir_exit;
    }
    g_dir_ops.exit();
}

// Opens a directory
inline static int dir_opendir(const char* path, int flags) {
    if (!g_dir_ops.opendir) {
        g_dir_ops.opendir = &dir_opendir;
    }
    return g_dir_ops.opendir(path, flags);
}

// Closes a directory
inline static int dir_closedir(int dirp) {
    if (!g_dir_ops.closedir) {
        g_dir_ops.closedir = &dir_closedir;
    }
    return g_dir_ops.closedir(dirp);
}

// Reads a directory entry
inline static struct dirent* dir_readdir(int dirp) {
    if (!g_dir_ops.readdir) {
        g_dir_ops.readdir = &dir_readdir;
    }
    return g_dir_ops.readdir(dirp);
}

// Seeks to a specific location in a directory
inline static int dir_seekdir(int dirp, off_t offset) {
    if (!g_dir_ops.seekdir) {
        g_dir_ops.seekdir = &dir_seekdir;
    }
    return g_dir_ops.seekdir(dirp, offset);
}

// Returns the current position in a directory
inline static int dir_telldir(int dirp) {
    if (!g_dir_ops.telldir) {
        g_dir_ops.telldir = &dir_telldir;
    }
    return g_dir_ops.telldir(dirp);
}

// Creates a new directory
inline static int dir_mkdir(const char* path, mode_t mode) {
    if (!g_dir_ops.mkdir) {
        g_dir_ops.mkdir = &dir_mkdir;
    }
    return g_dir_ops.mkdir(path, mode);
}

// Removes a directory
inline static int dir_rmdir(const char* path) {
    if (!g_dir_ops.rmdir) {
        g_dir_ops.rmdir = &dir_rmdir;
    }
    return g_dir_ops.rmdir(path);
}

// Renames a file or directory
inline static int dir_rename(const char* oldpath, const char* newpath) {
    if (!g_dir_ops.rename) {
        g_dir_ops.rename = &dir_rename;
    }
    return g_dir_ops.rename(oldpath, newpath);
}
