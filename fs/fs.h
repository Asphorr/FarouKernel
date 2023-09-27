#ifndef _FS_H
#define _FS_H

// File system types
typedef enum {
    FS_TYPE_UNKNOWN,
    FS_TYPE_FAT32,
    FS_TYPE_NTFS,
    FS_TYPE_EXT2,
    FS_TYPE_EXT3,
    FS_TYPE_EXT4,
} fs_type;

// File system operations
void fs_init(void);
int fs_mount(const char *device, const char *filesystem_type);
int fs_unmount(const char *device);
int fs_create(const char *path);
int fs_delete(const char *path);
int fs_read(const char *path, void **data, size_t *size);
int fs_write(const char *path, const void *data, size_t size);
int fs_get_info(const char *path, struct stat *st);

// File system data structures
struct fs_node {
    int type; // File or directory
    char name[MAX_NAME_LEN];
    char path[MAX_PATH_LEN];
    uint32_t size;
    uint32_t blocks;
    uint16_t flags;
};

struct fs_directory {
    struct fs_node node;
    struct fs_directory *parent;
    struct fs_directory *child;
    struct fs_directory *next;
    struct fs_directory *prev;
};

// Functions for working with files and directories
int fs_open(const char *path, int mode);
int fs_close(int fd);
ssize_t fs_read_from(int fd, void *buf, size_t count);
ssize_t fs_write_to(int fd, const void *buf, size_t count);
int fs_seek(int fd, off_t offset, int whence);
int fs_tell(int fd, off_t *offset);
int fs_mkdir(const char *path);
int fs_rmdir(const char *path);
int fs_rename(const char *old_name, const char *new_name);

#endif /* _FS_H */
