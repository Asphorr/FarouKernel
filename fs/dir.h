#ifndef _DIR_H
#define _DIR_H

#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <utime.h>

// Directory entry structure
typedef struct dirent {
ino_t d_ino; // Inode number
off_t d_off; // Offset to the start of the file
uint16_t d_reclen; // Length of this directory entry
uint8_t d_type; // Type of file (see below)
char d_name[256]; // Name of the file
} DirEntry;

// Types of files that can be stored in a directory
enum FileType {
REGULAR = 1, // Regular file
DIRECTORY = 2, // Directory
SYMLINK = 3, // Symbolic link
BLOCKDEV = 4, // Block device
CHARDEV = 5, // Character device
FIFOPipe = 6, // FIFO (named pipe)
SOCKET = 7, // Socket
};

// Function prototypes
void initDir();
void exitDir();
int openDir(const char *path, int flags);
int closeDir(int dirp);
DirEntry *readDir(int dirp);
int seekDir(int dirp, off_t offset);
int tellDir(int dirp);
int makeDir(const char *path, mode_t mode);
int removeDir(const char *path);
int renameFile(const char *oldPath, const char *newPath);

#endif // _DIR_H
