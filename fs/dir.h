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
struct dirent {
    ino_t d_ino;     /* Inode number */
    off_t d_off;   /* Offset to the start of the file */
    uint16_t d_reclen;  /* Length of this directory entry */
    uint8_t d_type;  /* Type of file (see below) */
    char d_name[256];  /* Name of the file */
};

// Types of files that can be stored in a directory
enum {
    DT_REG = 1,  // Regular file
    DT_DIR = 2,  // Directory
    DT_LNK = 3,  // Symbolic link
    DT_BLK = 4,  // Block device
    DT_CHR = 5,  // Character device
    DT_FIFO = 6,  // FIFO (named pipe)
    DT_SOCK = 7,  // Socket
};

// Function prototypes
void dir_init(void);
void dir_exit(void);
int dir_opendir(const char *path, int flags);
int dir_closedir(int dirp);
struct dirent *dir_readdir(int dirp);
int dir_seekdir(int dirp, off_t offset);
int dir_telldir(int dirp);
int dir_mkdir(const char *path, mode_t mode);
int dir_rmdir(const char *path);
int dir_rename(const char *oldpath, const char *newpath);

#endif  // _DIR_H
