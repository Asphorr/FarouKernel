#include <stdint.h>
#include <stddef.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

/* Define system call numbers */
#define SYS_WRITE    1
#define SYS_READ     2
#define SYS_OPEN     3
#define SYS_CLOSE    4
#define SYS_LSEEK    5
#define SYS_FSTAT    6
#define SYS_EXIT     60
/* Add more system call numbers as needed */

/* System call handlers */
size_t sys_write(int fd, const void *buf, size_t count);
size_t sys_read(int fd, void *buf, size_t count);
int sys_open(const char *path, int flags, int mode);
int sys_close(int fd);
off_t sys_lseek(int fd, off_t offset, int whence);
int sys_fstat(int fd, struct stat *st);
void sys_exit(int status);
/* Add more system call handlers as needed */

/* System call table */
typedef size_t (*syscall_handler_t)(uint64_t, uint64_t, uint64_t, uint64_t, uint64_t);
syscall_handler_t syscall_table[] = {
    NULL,
    (syscall_handler_t)sys_write,
    (syscall_handler_t)sys_read,
    (syscall_handler_t)sys_open,
    (syscall_handler_t)sys_close,
    (syscall_handler_t)sys_lseek,
    (syscall_handler_t)sys_fstat,
    NULL, /* Placeholder for future system calls */
    NULL, /* Placeholder for future system calls */
    NULL, /* Placeholder for future system calls */
    NULL, /* Placeholder for future system calls */
    (syscall_handler_t)sys_exit,
    /* Add more system call handlers as needed */
};

/* System call entry point */
size_t syscall_entry(uint64_t syscall_num, uint64_t arg1, uint64_t arg2, uint64_t arg3, uint64_t arg4) {
    if (syscall_num >= sizeof(syscall_table) / sizeof(syscall_table[0]) || syscall_table[syscall_num] == NULL) {
        errno = ENOSYS;
        return (size_t)-1;
    }

    return syscall_table[syscall_num](arg1, arg2, arg3, arg4, 0);
}

/* System call handlers implementation */
size_t sys_write(int fd, const void *buf, size_t count) {
    ssize_t result = write(fd, buf, count);
    if (result < 0) {
        return (size_t)-1;
    }
    return (size_t)result;
}

size_t sys_read(int fd, void *buf, size_t count) {
    ssize_t result = read(fd, buf, count);
    if (result < 0) {
        return (size_t)-1;
    }
    return (size_t)result;
}

int sys_open(const char *path, int flags, int mode) {
    int fd = open(path, flags, mode);
    if (fd < 0) {
        return -1;
    }
    return fd;
}

int sys_close(int fd) {
    int result = close(fd);
    if (result < 0) {
        return - 1;
       }
       return 0;
   }

   off_t sys_lseek(int fd, off_t offset, int whence) {
       off_t result = lseek(fd, offset, whence);
       if (result == (off_t)-1) {
           return (off_t)-1;
       }
       return result;
   }

   int sys_fstat(int fd, struct stat *st) {
       int result = fstat(fd, st);
       if (result < 0) {
           return -1;
       }
       return 0;
   }

   void sys_exit(int status) {
       _exit(status);
   }

   /* Add more system call handlers as needed */

   int main() {
       /* Test system call implementation */

       /* Test sys_open and sys_write */
       int fd = sys_open("testfile.txt", O_WRONLY | O_CREAT | O_TRUNC, 0644);
       if (fd != -1) {
           const char *message = "Hello, world!";
           sys_write(fd, message, 13);
           sys_close(fd);
       }

       /* Test sys_read */
       fd = sys_open("testfile.txt", O_RDONLY, 0);
       if (fd != -1) {
           char buffer[20];
           size_t bytes_read = sys_read(fd, buffer, sizeof(buffer) - 1);
           buffer[bytes_read] = '\0';
           sys_close(fd);
           printf("Read from file: %s\n", buffer);
       }

       /* Test sys_lseek and sys_read */
       fd = sys_open("testfile.txt", O_RDONLY, 0);
       if (fd != -1) {
           sys_lseek(fd, 7, SEEK_SET);
           char buffer[20];
           size_t bytes_read = sys_read(fd, buffer, sizeof(buffer) - 1);
           buffer[bytes_read] = '\0';
           printf("Read from offset 7: %s\n", buffer);
           sys_close(fd);
       }

       /* Test sys_fstat */
       fd = sys_open("testfile.txt", O_RDONLY, 0);
       if (fd != -1) {
           struct stat st;
           if (sys_fstat(fd, &st) == 0) {
               printf("File size: %lld bytes\n", (long long)st.st_size);
               printf("File mode: %o\n", st.st_mode & 0777);
           }
           sys_close(fd);
       }

       #include <stdint.h>

#define PAGE_PRESENT  0x1
#define PAGE_RW       0x2
#define PAGE_USER     0x4

/* Page Directory Entry (PDE) and Page Table Entry (PTE) structures */
typedef struct {
    uint32_t entries[1024];
} page_table_t;

typedef struct {
    uint32_t entries[1024];
} page_directory_t;

/* Kernel Page Directory */
page_directory_t kernel_page_directory __attribute__((aligned(4096)));
page_table_t kernel_page_table __attribute__((aligned(4096)));

/* Initialize Paging */
void init_paging() {
    /* Initialize Page Table */
    for (int i = 0; i < 1024; i++) {
        kernel_page_table.entries[i] = (i * 0x1000) | PAGE_PRESENT | PAGE_RW;
    }

    /* Initialize Page Directory */
    kernel_page_directory.entries[0] = ((uint32_t)&kernel_page_table) | PAGE_PRESENT | PAGE_RW;
    for (int i = 1; i < 1024; i++) {
        kernel_page_directory.entries[i] = 0;
    }

    /* Load Page Directory */
    asm volatile("mov %0, %%cr3":: "r"(&kernel_page_directory));

    /* Enable Paging */
    uint32_t cr0;
    asm volatile("mov %%cr0, %0": "=r"(cr0));
    cr0 |= 0x80000000;
    asm volatile("mov %0, %%cr0":: "r"(cr0));
}

void kernel_main() {
    init_paging();
    /* Further kernel initialization */
}
       #include <stdint.h>

#define HEAP_START 0x100000
#define HEAP_SIZE  0x100000

typedef struct free_block {
    size_t size;
    struct free_block *next;
} free_block_t;

static free_block_t *free_list = (free_block_t *)HEAP_START;

void init_heap() {
    free_list->size = HEAP_SIZE - sizeof(free_block_t);
    free_list->next = NULL;
}

void *kmalloc(size_t size) {
    free_block_t *current = free_list;
    free_block_t *previous = NULL;

    /* Align to 8 bytes */
    size = (size + 7) & ~7;

    while (current != NULL) {
        if (current->size >= size) {
            if (current->size > size + sizeof(free_block_t)) {
                /* Split the block */
                free_block_t *new_block = (free_block_t *)((uintptr_t)current + sizeof(free_block_t) + size);
                new_block->size = current->size - size - sizeof(free_block_t);
                new_block->next = current->next;
                current->size = size;
                current->next = new_block;
            }

            if (previous != NULL) {
                previous->next = current->next;
            } else {
                free_list = current->next;
            }

            return (void *)((uintptr_t)current + sizeof(free_block_t));
        }

        previous = current;
        current = current->next;
    }

    return NULL; /* Out of memory */
}

void kfree(void *ptr) {
    free_block_t *block = (free_block_t *)((uintptr_t)ptr - sizeof(free_block_t));
    block->next = free_list;
    free_list = block;
}

void kernel_main() {
    init_paging();
    init_heap();

    /* Test heap allocation */
    char *ptr1 = (char *)kmalloc(100);
    char *ptr2 = (char *)kmalloc(200);
    kfree(ptr1);
    kfree(ptr2);
    /* Further kernel initialization */
}
       
       /* Test sys_exit */
       printf("Exiting with code 0\n");
       sys_exit(0);

       

       return 0; /* This will not be reached due to sys_exit */
   }
