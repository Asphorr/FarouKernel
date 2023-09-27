#include "fs.h"

// Initialize the file system
void fs_init(void) {
    // Initialize the root directory
    root = malloc(sizeof(struct fs_directory));
    root->name = "/";
    root->path = "";
    root->size = 0;
    root->blocks = 0;
    root->flags = 0;
    root->parent = NULL;
    root->child = NULL;
    root->next = NULL;
    root->prev = NULL;

    // Create the root directory
    fs_mkdir("/");
}

// Mount a file system
int fs_mount(const char *device, const char *filesystem_type) {
    // Check if the device is valid
    if (!device || !filesystem_type) {
        return -EINVAL;
    }

    // Check if the file system type is supported
    if (strcmp(filesystem_type, "fat32") != 0 && strcmp(filesystem_type, "ntfs") != 0 && strcmp(filesystem_type, "ext2") != 0 && strcmp(filesystem_type, "ext3") != 0 && strcmp(filesystem_type, "ext4") != 0) {
        return -ENOTSUP;
    }

    // Mount the file system
    // TO DO: Implement mount code here
    return 0;
}

// Unmount a file system
int fs_unmount(const char *device) {
    // Check if the device is valid
    if (!device) {
        return -EINVAL;
    }

    // Unmount the file system
    // TO DO: Implement unmount code here
    return 0;
}

// Create a file
int fs_create(const char *path) {
    // Check if the path is valid
    if (!path) {
        return -EINVAL;
    }

    // Check if the parent directory exists
    struct fs_directory *parent = fs_get_directory(path, 0);
    if (!parent) {
        return -ENOENT;
    }

    // Allocate space for the new file
    struct fs_node *file = malloc(sizeof(struct fs_node));
    file->name = path + strlen(parent->path);
    file->path = path;
    file->size = 0;
    file->blocks = 0;
    file->flags = 0;

    // Add the file to the parent directory
    file->parent = parent;
    file->child = NULL;
    file->next = NULL;
    file->prev = NULL;
    parent->child = file;

    // Return the file descriptor
    return fs_open(path, O_RDWR | O_CREAT);
}

// Delete a file
int fs_delete(const char *path) {
    // Check if the path is valid
    if (!path) {
        return -EINVAL;
    }

    // Find the file to delete
    struct fs_node *file = fs_get_node(path);
    if (!file) {
        return -ENOENT;
    }

    // Remove the file from its parent directory
    file->parent->child = file->next;
    file->next = NULL;
    file->prev = NULL;

    // Free the file structure
    free(file);

    return 0;
}

// Read from a file
ssize_t fs_read_from(int fd, void *buf, size_t count) {
    // Check if the file descriptor is valid
    if (fd < 0) {
        return -EBADF;
    }

    // Check if the buffer is null
    if (!buf) {
        return -EFAULT;
    }

    // Check if the count is zero
    if (count == 0) {
        return 0;
    }

    // Find the file associated with the file descriptor
    struct fs_node *file = fs_get_node(fd);
    if (!file) {
        return -ENOENT;
    }

    // Calculate the number of bytes to read
ssize_t bytes_to_read = min(count, file->size - file->cursor);

// Read the data from the file
ssize_t ret = read(file->fd, buf, bytes_to_read);

// Update the file cursor
file->cursor += ret;

// Return the number of bytes read
return ret;
}

// Write to a file
ssize_t fs_write_to(int fd, const void *buf, size_t count) {
// Check if the file descriptor is valid
if (fd < 0) {
return -EBADF;
}

// Check if the buffer is null
if (!buf) {
    return -EFAULT;
}

// Check if the count is zero
if (count == 0) {
    return 0;
}

// Find the file associated with the file descriptor
struct fs_node *file = fs_get_node(fd);
if (!file) {
    return -ENOENT;
}

// Calculate the number of bytes to write
ssize_t bytes_to_write = min(count, file->size - file->cursor);

// Write the data to the file
ssize_t ret = write(file->fd, buf, bytes_to_write);

// Update the file cursor
file->cursor += ret;

// Return the number of bytes written
return ret;
}

// Get the file name from a file descriptor
char *fs_filename(int fd) {
// Check if the file descriptor is valid
if (fd < 0) {
return NULL;
}

// Find the file associated with the file descriptor
struct fs_node *file = fs_get_node(fd);
if (!file) {
    return NULL;
}

// Return the file name
return file->name;
}

// Get the file size
size_t fs_filesize(int fd) {
// Check if the file descriptor is valid
if (fd < 0) {
return -1;
}

// Find the file associated with the file descriptor
struct fs_node *file = fs_get_node(fd);
if (!file) {
    return -1;
}

// Return the file size
return file->size;
}

// Get the file creation time
time_t fs_creation_time(int fd) {
// Check if the file descriptor is valid
if (fd < 0) {
return -1;
}

// Find the file associated with the file descriptor
struct fs_node *file = fs_get_node(fd);
if (!file) {
    return -1;
}

// Return the file creation time
return file->ctime;
}

// Get the file modification time
time_t fs_modification_time(int fd) {
// Check if the file descriptor is valid
if (fd < 0) {
return -1;
}

// Find the file associated with the file descriptor
struct fs_node *file = fs_get_node(fd);
if (!file) {
    return -1;
}

// Return the file modification time
return file->mtime;
}

// Get the file permissions
mode_t fs_permissions(int fd) {
// Check if the file descriptor is valid
if (fd < 0) {
return -1;
}

// Find the file associated with the file descriptor
struct fs_node *file = fs_get_node(fd);
if (!file) {
    return -1;
}

// Return the file permissions
return file->mode;
}

// Set the file permissions
int fs_set_permissions(int fd, mode_t perm) {
// Check if the file descriptor is valid
if (fd < 0) {
return -1;
}

// Find the file associated with the file descriptor
struct fs_node *file = fs_get_node(fd);
if (!file) {
    return -1;
}

// Check if the user has permission to change the file permissions
if (fs_user_has_permission(file->uid, file->gid, file->mode) != 0) {
    return -EPERM;
}

// Change the file permissions
file->mode = perm;

// Update the file metadata
fs_update_metadata(file);

// Return success
return 0;
}

// Create a new file
int fs_create_file(const char *path, mode_t perm) {
    // Check if the path is valid
    if (path == NULL || strlen(path) == 0) {
        return -EINVAL;
    }

    // Check if the parent directory exists
    struct fs_node *parent = fs_get_node(fs_get_root_dir());
    if (!parent) {
        return -ENOENT;
    }

    // Check if the parent directory has enough space to store the new file
    if (parent->free_space < sizeof(struct fs_node)) {
        return -ENOSPC;
    }

    // Allocate memory for the new file node
    struct fs_node *file = malloc(sizeof(struct fs_node));
    if (!file) {
        return -ENOMEM;
    }

    // Initialize the file node
    file->name = strdup(path);
    file->uid = getuid();
    file->gid = getgid();
    file->mode = perm;
    file->size = 0;
    file->cursor = 0;
    file->ftype = FS_REGULAR;
    file->parent = parent;

    // Insert the new file into the parent directory
    fs_insert_node(parent, file);

    // Update the parent directory's free space
    parent->free_space -= sizeof(struct fs_node);

    // Return success
    return 0;
}

// Delete a file
int fs_delete_file(const char *path) {
    // Check if the path is valid
    if (path == NULL || strlen(path) == 0) {
        return -EINVAL;
    }

    // Find the file associated with the path
    struct fs_node *file = fs_get_node(path);
    if (!file) {
        return -ENOENT;
    }

    // Check if the file is a directory
    if (file->ftype == FS_DIRECTORY) {
        return -EISDIR;
    }

    // Decrease the link count of the file
    file->link_count--;

    // If the link count is 0, delete the file
    if (file->link_count <= 0) {
        // Free the memory allocated for the file node
        free(file);

        // Update the parent directory's free space
        file->parent->free_space += sizeof(struct fs_node);

        // Remove the file from the parent directory
        fs_remove_node(file->parent, file);

        // Return success
        return 0;
    } else {
        return -EEXIST;
    }
}

// Rename a file
int fs_rename_file(const char *old_path, const char *new_path) {
    // Check if the old path is valid
    if (old_path == NULL || strlen(old_path) == 0) {
        return -EINVAL;
    }

    // Check if the new path is valid
    if (new_path == NULL || strlen(new_path) == 0) {
        return -EINVAL;
    }

    // Find the file associated with the old path
    struct fs_node *old_file = fs_get_node(old_path);
    if (!old_file) {
        return -ENOENT;
    }

    // Check if the file is a directory
    if (old_file->ftype == FS_DIRECTORY) {
        return -EISDIR;
    }  

// Check if the file is a symbolic link
if (old_file->ftype == FS_SYMLINK) {
// Get the target of the symbolic link
struct fs_node *target = fs_get_node(old_file->data);
if (!target) {
return -ENOENT;
}

// Check if the target file exists
if (!target->exist) {
    return -ENOENT;
}

// Check if the target file is a directory
if (target->ftype == FS_DIRECTORY) {
    return -EISDIR;
}

// Rename the symbolic link
old_file->data = new_path;
old_file->ftype = FS_SYMLINK;
old_file->exist = 1;
old_file->updated = 1;

// Update the parent directory's cache
fs_update_cache(old_file->parent, old_file);

// Return success
return 0;
}

// Check if the file is a regular file
if (old_file->ftype == FS_REGULAR) {
// Check if the file is in use by another process
if (old_file->ref_count > 0) {
return -EBUSY;
}

// Rename the file
old_file->name = new_path;
old_file->updated = 1;

// Update the parent directory's cache
fs_update_cache(old_file->parent, old_file);

// Return success
return 0;
}

// File not found
return -ENOENT;
}

// Check if two files are equal
int fs_equal_files(struct fs_node *file1, struct fs_node *file2) {
// Compare the names
if (strcmp(file1->name, file2->name) != 0) {
return 0;
}

// Compare the modes
if (file1->mode != file2->mode) {
    return 0;
}

// Compare the uids
if (file1->uid != file2->uid) {
    return 0;
}

// Compare the gids
if (file1->gid != file2->gid) {
    return 0;
}

// Compare the sizes
if (file1->size != file2->size) {
    return 0;
}

// Compare the modification times
if (file1->mtime != file2->mtime) {
    return 0;
}

// Compare the device numbers
if (file1->dev != file2->dev) {
    return 0;
}

// Compare the inode numbers
if (file1->ino != file2->ino) {
    return 0;
}

// Files are equal
return 1;
}

// Check if a file is a descendant of another file
int fs_is_descendant(struct fs_node *ancestor, struct fs_node *descendant) {
// Check if the ancestor is a directory
if (ancestor->ftype != FS_DIRECTORY) {
return 0;
}

// Check if the descendant is a child of the ancestor
if (descendant->parent == ancestor) {
    return 1;
}

// Recursively check if the descendant is a child of a child of the ancestor
struct fs_node *child = descendant->parent;
while (child != NULL) {
    if (child == ancestor) {
        return 1;
    }
    child = child->parent;
}

// Not a descendant
return 0;
}

// Walk through the file system tree and call a function on each node
void fs_walk(struct fs_node *root, void (*func)(struct fs_node *)) {
// Base case: root is NULL
if (root == NULL) {
return;
}

    // Call the function on the current node
    func(root);

    // If the current node is a directory, recursively walk its children
    if (root->ftype == FS_DIRECTORY) {
        for (int i = 0; i < root->children_count; i++) {
            struct fs_node *child = root->children[i];
            fs_walk(child, func);
        }
    }
}

// Iterate over all nodes in the file system
for (int i = 0; i < fs->nodes_count; i++) {
    struct fs_node *node = fs->nodes[i];
    fs_walk(node, func);
}
