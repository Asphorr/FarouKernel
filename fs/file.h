#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

/* Maximum number of files supported by the program */
#define MAX_FILES 64

/* Structure to represent a file */
typedef struct file {
    char name[256];   /* Name of the file */
    int fd;           /* File descriptor */
    off_t size;       /* Size of the file in bytes */
    unsigned long cache_offset; /* Offset of the last cached block */
    unsigned long cache_length; /* Length of the last cached block */
    char* cache_data; /* Pointer to the data stored in the cache */
} file_t;

/* Array of file structures */
static file_t files[MAX_FILES];

/* Current index in the array of file structures */
static int current_index = 0;

/* Initialize the file system */
void init_filesystem() {
    /* Clear the array of file structures */
    memset(&files, 0, sizeof(files));
}

/* Create a new file */
int create_file(char* filename) {
    /* Check if there are any available slots in the array of file structures */
    if (current_index >= MAX_FILES) {
        errno = ENOSPC;
        return -1;
    }

    /* Create a new file structure */
    file_t* file = &files[current_index++];
    strcpy(file->name, filename);
    file->fd = -1;
    file->cache_offset = 0;
    file->cache_length = 0;
    file->cache_data = NULL;

    /* Return the index of the newly created file structure */
    return current_index - 1;
}

/* Delete a file */
int delete_file(int index) {
    /* Check if the given index is valid */
    if (index < 0 || index >= current_index) {
        errno = ENOENT;
        return -1;
    }

    /* Close the file descriptor associated with the file */
    if (files[index].fd != -1) {
        close(files[index].fd);
    }

    /* Remove the file structure from the array */
    memmove(&files[index], &files[index + 1], sizeof(file_t) * (current_index - index - 1));
    current_index--;

    /* Return success */
    return 0;
}

/* Open a file */
int open_file(char* filename, int flags) {
    /* Search for the file in the array of file structures */
    for (int i = 0; i < current_index; i++) {
        if (strcmp(files[i].name, filename) == 0) {
            /* If the file was found, check if it's already opened */
            if (flags & O_CREAT && files[i].fd != -1) {
                errno = EEXIST;
                return -1;
            } else {
                /* If the file wasn't already opened, open it now */
                files[i].fd = open(filename, flags);
                if (files[i].fd == -1) {
                    perror("open");
                    return -1;
                }

                /* Update the size of the file */
                files[i].size = lseek(files[i].fd, 0, SEEK_END);
                lseek(files[i].fd, 0, SEEK_SET);

                /* Return the index of the file structure */
                return i;
            }
        }
    }

    /* If the file wasn't found, create a new one */
    int index = create_file(filename);
    if (index == -1) {
        perror("create_file");
        return -1;
    }

    /* Open the file */
    files[index].fd = open(filename, flags | O_RDONLY);
    if (files[index].fd == -1) {
        perror("open");
        return -1;
    }

    /* Update the size of the file */
    files[index].size = lseek(files[index].fd, 0, SEEK_END);
    lseek(files[index].fd, 0, SEEK_SET);

    /* Return the index of the file structure */
    return index;
}

/* Close a file */
int close_file(int index) {
    /* Check if the given index is valid */
    if (index < 0 || index >= current_index) {
        errno = EBADF;
        return -1;
    }
