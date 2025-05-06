#include "fs.h"

#define MAX_FILESYSTEMS 8
#define MAX_FILES 512
#define MAX_DIRS 512
#define MAX_BLOCKS 1024
#define BLOCK_SIZE 512

static struct fs_node *g_file_systems[MAX_FILESYSTEMS];
static struct fs_node g_files[MAX_FILES];
static struct fs_node g_directories[MAX_DIRS];
static unsigned long g_blocks[MAX_BLOCKS];

static inline int get_free_index() {
    static int index = -1;
    while (++index < MAX_BLOCKS && g_blocks[index]) {}
    return index >= MAX_BLOCKS ? -1 : index;
}

static inline void free_index(int index) {
    g_blocks[index] = 0;
}

static inline struct fs_node* alloc_node(enum fs_type type) {
    switch (type) {
        case FS_TYPE_UNKNOWN:
            break;
        case FS_TYPE_FAT32:
            return &g_files[get_free_index()];
        case FS_TYPE_NTFS:
            return &g_directories[get_free_index()];
        default:
            break;
    }
    return NULL;
}

static inline void free_node(struct fs_node *node) {
    free_index((unsigned long)node / sizeof(*node));
}

static inline struct fs_node* find_node(const char *path) {
    struct fs_node *node = g_file_systems[0];
    while (node && strcmp(node->name, path) != 0) {
        node = node->next;
    }
    return node;
}

static inline struct fs_node* create_node(const char *path, enum fs_type type) {
    struct fs_node *node = alloc_node(type);
    if (!node) {
        return NULL;
    }
    memset(node, 0, sizeof(*node));
    node->type = type;
    strcpy(node->name, path);
    node->flags |= FS_FLAG_NEW;
    return node;
}

static inline void delete_node(struct fs_node *node) {
    free_node(node);
}

static inline int mount_filesystem(const char *device, const char *filesystem_type) {
    struct fs_node *node = find_node(device);
    if (!node || !strcmp(node->name, "/")) {
        return -ENOENT;
    }
    if (node->type != FS_TYPE_UNKNOWN) {
        return -EBUSY;
    }
    node->type = filesystem_type;
    node->flags &= ~FS_FLAG_MOUNTED;
    return 0;
}

static inline int unmount_filesystem(const char *device) {
    struct fs_node *node = find_node(device);
    if (!node || !strcmp(node->name, "/") || node->type == FS_TYPE_UNKNOWN) {
        return -ENOENT;
    }
    node->type = FS_TYPE_UNKNOWN;
    node->flags &= ~FS_FLAG_MOUNTED;
    return 0;
}

static inline int create_file(const char *path) {
    struct fs_node *node = create_node(path, FS_TYPE_FAT32);
    if (!node) {
        return -ENOSPC;
    }
    node->flags |= FS_FLAG_CREATED;
    return 0;
}

static inline int delete_file(const char *path) {
    struct fs_node *node = find_node(path);
    if (!node || node->type != FS_TYPE_FAT32) {
        return -ENOENT;
    }
    delete_node(node);
    return 0;
}

static inline ssize_t read_file(const char *path, void **data, size_t *size) {
    struct fs_node *node = find_node(path);
    if (!node || node->type != FS_TYPE_FAT32) {
        return -ENOENT;
    }
    *data = malloc(node->size);
    if (!*data) {
        return -ENOMEM;
    }
    memcpy(*data, node->buffer, node->size);
    *size = node->size;
    return 0;
}

static inline ssize_t write_file(const char *path, const void *data, size_t size) {
    struct fs_node *node = find_node(path);
    if (!node || node->type != FS_TYPE_FAT32) {
        return -ENOENT;
    }
    if (size > node->size) {
        return -ENOSPC;
    }
    memcpy(node->buffer, data, size);
    node->size = size;
    return 0;
}

static inline int get_file_info(const char *path, struct stat *st) {
    struct fs_node *node = find_node(path);
    if (!node || node->type != FS_TYPE_FAT32) {
        return -ENOENT;
    }
    st->st_dev = 0;
    st->st_ino = 0;
    st->st_mode = S_IFREG | 0777;
    st->st_nlink = 1;
    st->st_uid = 0;
    st->st_gid = 0;
    st->st_rdev = 0;
    st->st_size = node->size;
    st->st_blksize = BLOCK_SIZE;
    st->st_blocks = node->blocks;
    return 0;
}

static inline int mkdir(const char *path) {
    struct fs_node *node = create_node(path, FS_TYPE_NTFS);
    if (!node) {
        return -ENOSPC;
    }
    node->flags |= FS_FLAG_CREATED;
    return 0;
}

static inline int rmdir(const char *path) {
    struct fs_node *node = find_node(path);
    if (!node || node->type != FS_TYPE_NTFS) {
        return -ENOENT;
    }
    delete_node(node);
    return 0;
}

static inline int rename(const char *old_name, const char *new_name) {
    struct fs_node *node = find_node(old_name);
    if (!node) {
        return -ENOENT;
    }
    strncpy(node->name, new_name, MAX_NAME_LEN);
    return 0;
}

static inline int open(const char *path, int mode) {
    struct fs_node *node = find_node(path);
    if (!node) {
        return -ENOENT;
    }
    if ((mode & O_CREATE && !(node->flags & FS_FLAG_CREATED)) {
return -EEXIST;
}
if ((mode & O_DIRECTORY) && node->type != FS_TYPE_NTFS) {
return -ENOTDIR;
}
if ((!(mode & O_DIRECTORY) && node->type != FS_TYPE_FAT32)) {
return -EISDIR;
}
return 0;
}


