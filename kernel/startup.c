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
#define PF_R 0x1
#define PF_W 0x2
#define PF_X 0x4

struct boot_params {
    uint32_t magic;
    uint32_t size;
    uint32_t load_addr;
    uint32_t entry_point;
};

void exit_with_error(const char *message) {
    perror(message);
    exit(EXIT_FAILURE);
}

char *map_file(const char *filename, size_t *size, int *fd) {
    *fd = open(filename, O_RDONLY);
    if (*fd < 0) {
        exit_with_error("Error opening file");
    }

    struct stat st;
    if (fstat(*fd, &st) < 0) {
        close(*fd);
        exit_with_error("Error getting file size");
    }

    *size = st.st_size;
    char *buffer = mmap(NULL, *size, PROT_READ, MAP_PRIVATE, *fd, 0);
    if (buffer == MAP_FAILED) {
        close(*fd);
        exit_with_error("Error mapping file");
    }

    return buffer;
}

void unmap_file(char *buffer, size_t size) {
    if (munmap(buffer, size) < 0) {
        exit_with_error("Error unmapping memory");
    }
}

void load_kernel_segments(const Elf32_Ehdr *ehdr, int fd, const char *buffer, size_t file_size) {
    size_t page_size = sysconf(_SC_PAGESIZE);
    Elf32_Phdr *phdr = (Elf32_Phdr *)(buffer + ehdr->e_phoff);
    for (int i = 0; i < ehdr->e_phnum; ++i) {
        if (phdr[i].p_type == PT_LOAD) {
            void *va = (void *)phdr[i].p_vaddr;
            size_t filesz = phdr[i].p_filesz;
            size_t memsz = phdr[i].p_memsz;

            if (phdr[i].p_offset + filesz > file_size) {
                exit_with_error("Segment extends beyond file size");
            }

            int prot = 0;
            if (phdr[i].p_flags & PF_R) prot |= PROT_READ;
            if (phdr[i].p_flags & PF_W) prot |= PROT_WRITE;
            if (phdr[i].p_flags & PF_X) prot |= PROT_EXEC;

            size_t start_page = (size_t)va / page_size * page_size;
            size_t end_page = ((size_t)va + memsz + page_size - 1) / page_size * page_size;
            size_t map_size = end_page - start_page;

            if (mmap((void *)start_page, map_size, prot, MAP_FIXED | MAP_ANONYMOUS | MAP_PRIVATE, -1, 0) == MAP_FAILED) {
                exit_with_error("Error mapping segment pages");
            }

            if (filesz > 0) {
                if (lseek(fd, phdr[i].p_offset, SEEK_SET) == -1) {
                    exit_with_error("Error seeking file");
                }
                if (read(fd, va, filesz) != filesz) {
                    exit_with_error("Error reading file");
                }
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
}

void load_kernel(const char *filename) {
    int fd;
    size_t file_size;
    char *buffer = map_file(filename, &file_size, &fd);

    Elf32_Ehdr *ehdr = (Elf32_Ehdr *)buffer;
    if (memcmp(ehdr->e_ident, ELFMAG, SELFMAG) != 0) {
        unmap_file(buffer, file_size);
        close(fd);
        fprintf(stderr, "Invalid ELF header\n");
        exit(EXIT_FAILURE);
    }

    load_kernel_segments(ehdr, fd, buffer, file_size);
    start_kernel(ehdr, buffer);
    unmap_file(buffer, file_size);
    close(fd);
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
