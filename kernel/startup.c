#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <unistd.h>

#define KERNEL_BASE_ADDR 0xC0000000
#define KERNEL_END_ADDR  0xFFFFFFFF
#define USERSPACE_BASE_ADDR 0x40000000
#define USERSPACE_END_ADDR  0x7FFFFFFF

typedef struct {
    uint32_t magic;
    uint32_t size;
    uint32_t load_addr;
    uint32_t entry_point;
} __attribute__((packed)) boot_params_t;

static inline void *get_boot_param(const char *name) {
    return (void *)(KERNEL_BASE_ADDR + strlen(name) + 1);
}

static int parse_elf64_header(Elf64_Ehdr *ehdr) {
    if (ehdr->e_ident[EI_CLASS] != ELFCLASS64 || ehdr->e_machine != EM_X86_64) {
        printf("Invalid ELF file format or architecture\n");
        return -1;
    }

    Elf64_Phdr *phdr = (Elf64_Phdr *)((char *)ehdr + ehdr->e_phoff);
    for (int i = 0; i < ehdr->e_phnum; ++i) {
        if (phdr[i].p_type == PT_LOAD && phdr[i].p_flags == (PF_R | PF_W | PF_X)) {
            uintptr_t start = phdr[i].p_vaddr;
            uintptr_t end = start + phdr[i].p_memsz;
            if (start >= USERSPACE_BASE_ADDR && end <= USERSPACE_END_ADDR) {
                continue;
            } else if (start >= KERNEL_BASE_ADDR && end <= KERNEL_END_ADDR) {
                continue;
            }
            printf("Invalid address range in ELF file\n");
            return -1;
        }
    }

    return 0;
}

static int map_elf64_file(FILE *fp, const char *filename) {
    Elf64_Ehdr ehdr;
    fread(&ehdr, sizeof(Elf64_Ehdr), 1, fp);
    if (parse_elf64_header(&ehdr) < 0) {
        return -1;
    }

    Elf64_Phdr *phdr = (Elf64_Phdr *)malloc(sizeof(Elf64_Phdr) * ehdr.e_phnum);
    if (!phdr) {
        perror("failed to allocate memory for ELF program headers");
        return -1;
    }
    memcpy(phdr, (char *)&ehdr + ehdr.e_phoff, sizeof(Elf64_Phdr) * ehdr.e_phnum);

    for (int i = 0; i < ehdr.e_phnum; ++i) {
        if (phdr[i].p_type != PT_LOAD) {
            continue;
        }

        uintptr_t start = phdr[i].p_vaddr;
        uintptr_t end = start + phdr[i].p_memsz;
        if (start >= USERSPACE_BASE_ADDR && end <= USERSPACE_END_ADDR) {
            continue;
        } else if (start >= KERNEL_BASE_ADDR && end <= KERNEL_END_ADDR) {
            continue;
        }

        void *buf = mmap((void *)start, phdr[i].p_memsz, PROT_READ | PROT_WRITE, MAP_FIXED | MAP_PRIVATE, fileno(fp), phdr[i].p_offset);
        if (buf == MAP_FAILED) {
            perror("failed to map ELF file into memory");
            free(phdr);
            return -1;
        }
    }

    free(phdr);
    return 0;
}

int main(int argc, char **argv) {
    FILE *fp = fopen(argv[1], "rb");
    if (!fp) {
        perror("failed to open input file");
        return 1;
    }

    if (map_elf64_file(fp, argv[1]) < 0) {
        fclose(fp);
        return 1;
    }

    fclose(fp);
    return 0;
}
