#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>
#include <time.h>

#include "platform.h"

// Function to get the current timestamp
uint64_t get_timestamp() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000 + (uint64_t)ts.tv_nsec / 1000;
}

// Function to read the contents of a file
char* read_file(const char* filename) {
    int fd = open(filename, O_RDONLY);
    if (fd == -1) {
        perror("Error opening file");
        return NULL;
    }

    struct stat sb;
    fstat(fd, &sb);

    char* contents = mmap(NULL, sb.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
    if (contents == MAP_FAILED) {
        perror("Error mapping file");
        close(fd);
        return NULL;
    }

    close(fd);
    return contents;
}

// Function to write the contents of a file
int write_file(const char* filename, const char* contents) {
    int fd = open(filename, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd == -1) {
        perror("Error creating file");
        return -1;
    }

    struct stat sb;
    fstat(fd, &sb);

    char* buffer = mmap(NULL, sb.st_size, PROT_WRITE, MAP_PRIVATE, fd, 0);
    if (buffer == MAP_FAILED) {
        perror("Error mapping file");
        close(fd);
        return -1;
    }

    strcpy(buffer, contents);
    munmap(buffer, sb.st_size);

    close(fd);
    return 0;
}

// Function to execute a command
int execute(const char* command) {
    pid_t pid = fork();
    if (pid == 0) {
        execvp(command, (char**)command);
        _Exit(EXIT_FAILURE);
    } else if (pid > 0) {
        waitpid(pid, NULL, 0);
        return 0;
    } else {
        perror("Error executing command");
        return -1;
    }
}

// Function to get the current working directory
char* get_cwd() {
    char buff[PATH_MAX];
    getcwd(buff, sizeof(buff));
    return buff;
}

// Function to create a new directory
int make_dir(const char* dirname) {
    mode_t mode = S_IRWXU | S_IRWXG | S_IRWXO;
    return mkdir(dirname, mode);
}

// Function to delete a directory
int remove_dir(const char* dirname) {
    return rmdir(dirname);
}

// Function to create a symbolic link
int symlink(const char* oldname, const char* newname) {
    return symlink(oldname, newname);
}

// Function to read the contents of a symbolic link
char* readlink(const char* filename) {
    int len = readlink(filename, NULL, 0);
    char* buffer = malloc(len + 1);
    readlink(filename, buffer, len + 1);
    return buffer;
}

// Function to get the user ID
uid_t get_uid() {
    return geteuid();
}

// Function to get the group ID
gid_t get_gid() {
    return getegid();
// Function to get the home directory of the current user
char* get_home_directory() {
    uid_t uid = geteuid();
    struct passwd* pwd = getpwuid(uid);
    return pwd->pw_dir;
}

// Function to check if a file exists
int file_exists(const char* filename) {
    struct stat sb;
    return stat(filename, &sb) == 0;
}

// Function to check if a directory exists
int directory_exists(const char* dirname) {
    struct stat sb;
    return stat(dirname, &sb) == 0 && S_ISDIR(sb.st_mode);
}

// Function to create a new file
int create_file(const char* filename) {
    int fd = open(filename, O_CREAT | O_RDWR | O_TRUNC, 0644);
    if (fd == -1) {
        perror("Error creating file");
        return -1;
    }
    close(fd);
    return 0;
}

// Function to delete a file
int delete_file(const char* filename) {
    return unlink(filename);
}

// Function to rename a file
int rename_file(const char* oldname, const char* newname) {
    return renamer(oldname, newname);
}

// Function to copy a file
int copy_file(const char* source, const char* destination) {
    int src_fd = open(source, O_RDONLY, 0);
    if (src_fd == -1) {
        perror("Error reading source file");
        return -1;
    }

    int dst_fd = open(destination, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (dst_fd == -1) {
        perror("Error writing destination file");
        close(src_fd);
        return -1;
    }

    struct stat sb;
    fstat(src_fd, &sb);

    char* buffer = mmap(NULL, sb.st_size, PROT_READ, MAP_PRIVATE, src_fd, 0);
    if (buffer == MAP_FAILED) {
        perror("Error mapping source file");
        close(src_fd);
        close(dst_fd);
        return -1;
    }

    off_t offset = 0;
    while (offset < sb.st_size) {
        ssize_t written = write(dst_fd, buffer + offset, sb.st_size - offset);
        if (written <= 0) {
            perror("Error writing destination file");
            break;
        }
        offset += written;
    }

    munmap(buffer, sb.st_size);
    close(src_fd);
    close(dst_fd);
    return 0;
}

// Function to move a file
int move_file(const char* source, const char* destination) {
    return rename(source, destination);
}

// Function to get the file size
off_t get_file_size(const char* filename) {
    struct stat sb;
    stat(filename, &sb);
    return sb.st_size;
}

// Function to get the last modified time
struct timespec get_last_modified_time(const char* filename) {
    struct stat sb;
    stat(filename, &sb);
    return sb.st_mtim;
}

// Function to get the file type
int get_file_type(const char* filename) {
    struct stat sb;
    stat(filename, &sb);
    switch (sb.st_mode & S_IFMT) {
        case S_IFREG:
            return FILE_TYPE_REGULAR;
        case S_IFLNK:
            return FILE_TYPE_SYMBOLIC_LINK;
        case S_IFDIR:
            return FILE_TYPE_DIRECTORY;
        default:
            return FILE_TYPE_UNKNOWN;
    }
}
