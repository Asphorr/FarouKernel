#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <elf.h>
#include <errno.h>

#define STACK_SIZE 0x1000
#define HEAP_SIZE 0x1000

struct boot_params {
    uint32_t magic;
    uint32_t size;
    uint32_t load_addr;
    uint32_t entry_point;
};

// Function to handle exiting with an error message
void exit_with_error(const char *message) {
    perror(message);
    exit(EXIT_FAILURE);
}

// Maps a file into memory and returns the address
char *map_file(const char *filename, size_t *size) {
    int fd = open(filename, O_RDONLY);
    if (fd < 0) {
        exit_with_error("Error opening file");
    }

    struct stat st;
    if (fstat(fd, &st) < 0) {
        close(fd);
        exit_with_error("Error getting file size");
    }

    *size = st.st_size;
    char *buffer = mmap(NULL, *size, PROT_READ, MAP_PRIVATE, fd, 0);
    if (buffer == MAP_FAILED) {
        close(fd);
        exit_with_error("Error mapping file");
    }

    close(fd);
    return buffer;
}

// Unmaps memory and checks for errors
void unmap_file(char *buffer, size_t size) {
    if (munmap(buffer, size) < 0) {
        exit_with_error("Error unmapping memory");
    }
}

void load_kernel_segments(const Elf32_Ehdr *ehdr, int fd, const char *buffer) {
    Elf32_Phdr *phdr = (Elf32_Phdr *)(buffer + ehdr->e_phoff);
    for (int i = 0; i < ehdr->e_phnum; ++i) {
        if (phdr[i].p_type == PT_LOAD) {
            void *va = (void *)phdr[i].p_vaddr;
            size_t size = phdr[i].p_filesz;
            if (mmap(va, size, PROT_READ | PROT_WRITE | PROT_EXEC, MAP_FIXED | MAP_PRIVATE, fd, phdr[i].p_offset) == MAP_FAILED) {
                exit_with_error("Error mapping segment");
            }
        }
    }
}

void start_kernel(const Elf32_Ehdr *ehdr, const char *buffer) {
    struct boot_params params = {
        .magic = 0x1BADB002,
        .size = sizeof(params),
        .load_addr = (uint32_t)buffer,
        .entry_point = ehdr->e_entry
    };

    void (*kernel_entry)(struct boot_params *) = (void (*)(struct boot_params *))ehdr->e_entry;
    kernel_entry(&params);
}

void setup_memory() {
    void *stack = malloc(STACK_SIZE);
    void *heap = malloc(HEAP_SIZE);

    if (!stack || !heap) {
        fprintf(stderr, "Memory allocation failed\n");
        exit(EXIT_FAILURE);
    }

    // Assume `current_process` structure available in the context
    // current_process.stack = stack;
    // current_process.heap = heap;
}

void load_kernel(const char *filename) {
    size_t file_size;
    char *buffer = map_file(filename, &file_size);
  
    Elf32_Ehdr *ehdr = (Elf32_Ehdr *)buffer;
    if (memcmp(ehdr->e_ident, ELFMAG, SELFMAG) != 0) {
        unmap_file(buffer, file_size);
        fprintf(stderr, "Invalid ELF header\n");
        exit(EXIT_FAILURE);
    }
  
    load_kernel_segments(ehdr, -1, buffer);
    start_kernel(ehdr, buffer);
    unmap_file(buffer, file_size);
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <kernel_file>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    setup_memory();
    load_kernel(argv[1]);

    return 0;
}