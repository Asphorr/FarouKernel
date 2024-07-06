#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <elf.h>

#define STACK_SIZE 0x1000
#define HEAP_SIZE 0x1000

struct boot_params {
    uint32_t magic;
    uint32_t size;
    uint32_t load_addr;
    uint32_t entry_point;
};

void load_kernel(const char *filename) {
    int fd = open(filename, O_RDONLY);
    if (fd < 0) {
        perror("open");
        exit(EXIT_FAILURE);
    }

    // Получение размера файла
    struct stat st;
    if (fstat(fd, &st) < 0) {
        perror("fstat");
        exit(EXIT_FAILURE);
    }

    // Отображение ELF-файла в память
    char *buffer = mmap(NULL, st.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
    if (buffer == MAP_FAILED) {
        perror("mmap");
        exit(EXIT_FAILURE);
    }

    close(fd);

    // Проверка заголовка ELF
    Elf32_Ehdr *ehdr = (Elf32_Ehdr *)buffer;
    if (memcmp(ehdr->e_ident, ELFMAG, SELFMAG) != 0) {
        fprintf(stderr, "Invalid ELF header\n");
        exit(EXIT_FAILURE);
    }

    // Загрузка сегментов PT_LOAD
    Elf32_Phdr *phdr = (Elf32_Phdr *)(buffer + ehdr->e_phoff);
    for (int i = 0; i < ehdr->e_phnum; ++i) {
        if (phdr[i].p_type == PT_LOAD) {
            void *va = (void *)phdr[i].p_vaddr;
            size_t size = phdr[i].p_filesz;
            void *mapped = mmap(va, size, PROT_READ | PROT_WRITE | PROT_EXEC, MAP_FIXED | MAP_PRIVATE, fd, phdr[i].p_offset);
            if (mapped == MAP_FAILED) {
                perror("mmap");
                exit(EXIT_FAILURE);
            }
        }
    }

    // Параметры загрузки ядра
    struct boot_params params = {
        .magic = 0x1BADB002,
        .size = sizeof(params),
        .load_addr = (uint32_t)buffer,
        .entry_point = ehdr->e_entry
    };

    // Запуск ядра
    void (*kernel_entry)(struct boot_params *) = (void (*)(struct boot_params *))ehdr->e_entry;
    kernel_entry(&params);

    // Освобождение памяти
    munmap(buffer, st.st_size);
}

void setup_memory() {
    void *stack = malloc(STACK_SIZE);
    void *heap = malloc(HEAP_SIZE);

    if (!stack || !heap) {
        fprintf(stderr, "Memory allocation failed\n");
        exit(EXIT_FAILURE);
    }

    // Инициализация указателей стека и кучи в текущем процессе
    // current_process.stack = stack; // Пример, как можно было бы использовать
    // current_process.heap = heap;   // Пример, как можно было бы использовать
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
