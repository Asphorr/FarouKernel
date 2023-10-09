#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "unistd.h"
#include "sys/types.h"
#include "fcntl.h"
#include "errno.h"

#define BLOCK_SIZE 4096
#define CACHE_SIZE 8192
#define MAX_FILES 32

typedef struct {
off_t offset; // Offset of the first byte in the cache
ssize_t length; // Number of bytes currently stored in the cache
char* data; // Pointer to the data stored in the cache
} cache_t;

// File structure containing information about an open file
typedef struct {
char name[256]; // Name of the file
int fd; // File descriptor associated with the file
off_t size; // Size of the file
cache_t cache; // Cache used by the file
} file_t;

// Array of file structures
static file_t files[MAX_FILES];

// Current index in the array of file structures
static int current_index = 0;

// Initializes the file system
void init_filesystem() {
// Clears the array of file structures
memset(&files, 0, sizeof(files));
}

// Creates a new file and returns its index or -1 on error
int create_file(const char* filename) {
// Checks if there are any available slots in the array of file structures
if (current_index >= MAX_FILES) {
errno = ENOSPC;
return -1;
}

// Creates a new file structure
file_t* file = &files[current_index++];
strncpy(file->name, filename, sizeof(file->name)-1);
file->fd = -1;
file->size = 0;
file->cache.offset = 0;
file->cache.length = 0;
file->cache.data = malloc(CACHE_SIZE);
if (!file->cache.data) {
    perror("malloc");
    return -1;
}

// Returns the index of the newly created file structure
return current_index-1;
}

// Deletes a file and returns 0 on success or -1 on error
int delete_file(int index) {
// Checks if the given index is valid
if (index < 0 || index >= current_index) {
errno = ENOENT;
return -1;
}

// Closes the file descriptor associated with the file
if (files[index].fd != -1) {
    close(files[index].fd);
}

// Removes the file structure from the array
free(files[index].cache.data);
memmove(&files[index], &files[index+1], sizeof(file_t)*(current_index-index-1));
current_index--;

// Returns success
return 0;
}

// Opens a file and returns its index or -1 on error
int open_file(const char* filename, int flags) {
// Searches for the file in the array of file structures
for (int i=0; i<current_index; ++i) {
if (strcmp(files[i].name, filename)==0) {
// If the file was found, checks if it's already opened
if ((flags&O_CREAT)&&(files[i].fd!=-1)) {
errno = EEXIST;
return -1;
} else {
// If the file wasn't already opened, opens it now
files[i].fd = open(filename, flags|O_RDWR);
if (files[i].fd == -1) {
perror("open");
return -1;
}

            // Updates the size of the file
            files[i].size = lseek(files[i].fd, 0, SEEK_END);
            lseek(files[i].fd, 0, SEEK_SET);

            // Returns the index of the file structure
            return i;
        }
    }
}

// If the file wasn't found, creates a new one
int index = create_file(filename);
if (index == -1) {
    perror("create_file");
    return -1;
}

// Opens the file
files[index].fd = open(filename, flags|O_RDWR);
if (files[index].fd == -1) {
    perror("open");
    return -1;
}

// Updates the size of the file
files[index].size = lseek(files[index].fd, 0, SEEK_END);
lseek(files[index].fd, 0, SEEK_SET);

// Returns the index of the file structure
return index;
}

// Reads count bytes starting at position pos into buf from the file specified by index
ssize_t read_from_file(int index, void* buf, size_t count, off_t pos) {
// Checks if the given index is valid
if (index < 0 || index >= current_index) {
errno = EBADF;
return -1;
}

// Calculates the number of blocks that need to be read
const off_t start_block = pos / BLOCK_SIZE;
const off_t end_block = (pos + count - 1) / BLOCK_SIZE;
const size_t num_blocks = end_block - start_block + 1;

// Allocates memory for reading the blocks
char** blocks = calloc(num_blocks, sizeof(*blocks));
if (!blocks) {
    perror("calloc");
    return -1;
}

// Loads all necessary blocks into memory
for (off_t b=start_block; b<=end_block; ++b) {
    // Checks if the requested block is already loaded in the cache
    if (files[index].cache.offset <= b && b < files[index].cache.offset + CACHE_SIZE / BLOCK_SIZE ) {
const ssize_t offset = (b - files[index].cache.offset) * BLOCK_SIZE;
memcpy(buf + (b - start_block) * BLOCK_SIZE, files[index].cache.data + offset, MIN(count, BLOCK_SIZE));
} else {
// Otherwise reads the block from disk
blocks[b - start_block] = malloc(BLOCK_SIZE);
if (!blocks[b - start_block]) {
perror("malloc");
free_read_buffers(blocks, num_blocks);
return -1;
}
pread(files[index].fd, blocks[b - start_block], BLOCK_SIZE, b * BLOCK_SIZE);
}
}

// Copies data from the buffer allocated above into buf
size_t total_bytes_copied = 0;
for (off_t b=start_block; b<=end_block; ++b) {
const size_t bytes_to_copy = MIN((b - start_block + 1) * BLOCK_SIZE - MAX(0, pos % BLOCK_SIZE), count - total_bytes_copied);
memcpy(buf + total_bytes_copied, blocks[b - start_block] ? blocks[b - start_block] : files[index].cache.data + (b - files[index].cache.offset) * BLOCK_SIZE, bytes_to_copy);
total_bytes_copied += bytes_to_copy;
}

// Frees the buffers used for loading the blocks
free_read_buffers(blocks, num_blocks);

// Returns the number of bytes copied
return total_bytes_copied;
}

// Writes count bytes starting at position pos from buf into the file specified by index
ssize_t write_into_file(int index, const void* buf, size_t count, off_t pos) {
// Checks if the given index is valid
if (index < 0 || index >= current_index) {
errno = EBADF;
return -1;
}

// Calculates the number of blocks that need to be written
const off_t start_block = pos / BLOCK_SIZE;
const off_t end_block = (pos + count - 1) / BLOCK_SIZE;
const size_t num_blocks = end_block - start_block + 1;

// Allocates memory for writing the blocks
char** blocks = calloc(num_blocks, sizeof(*blocks));
if (!blocks) {
perror("calloc");
return -1;
}

// Fills out the contents of each block
size_t total_bytes_written = 0;
for (off_t b=start_block; b<=end_block; ++b) {
// Checks if the requested block is already loaded in the cache
if (files[index].cache.offset <= b && b < files[index].cache.offset + CACHE_SIZE / BLOCK_SIZE) {
const ssize_t offset = (b - files[index].cache.offset) * BLOCK_SIZE;
memcpy(files[index].cache.data + offset, buf + (b - start_block) * BLOCK_SIZE, MIN(count, BLOCK_SIZE));
} else {
// Otherwise allocates space for the block
blocks[b - start_block] = malloc(BLOCK_SIZE);
if (!blocks[b - start_block]) {
perror("malloc");
free_write_buffers(blocks, num_blocks);
return -1;
}

    // And copies the appropriate part of buf into it
    const size_t bytes_to_copy = MIN((b - start_block + 1) * BLOCK_SIZE - MAX(0, pos % BLOCK_SIZE), count - total_bytes_written);
    memcpy(blocks[b - start_block], buf + total_bytes_written, bytes_to_copy);
    total_bytes_written += bytes_to_copy;
}
}

// Flushes any dirty blocks in the cache before overwriting them
flush_writes(index);

// Overwrites the blocks on disk
for (off_t b=start_block; b<=end_block; ++b) {
pwrite(files[index].fd, blocks[b - start_block] ? blocks[b - start_block] : files[index].cache.data + (b - files[index].cache.offset) * BLOCK_SIZE, BLOCK_SIZE, b * BLOCK_SIZE);
}

// Frees the buffers used for holding the blocks
free_write_buffers(blocks, num_blocks);

// Updates the size of the file if necessary
if (pos + count > files[index].size) {
files[index].size = pos + count;
}

// Returns the number of bytes written
return total_bytes_written;
}
\end{
