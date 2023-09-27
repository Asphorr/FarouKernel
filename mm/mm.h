#ifndef MM_H
#define MM_H

// Memory management functions
void *malloc(size_t size);
void free(void *ptr);
void *realloc(void *ptr, size_t size);

// Virtual memory functions
void *mmap(void *addr, size_t length, int prot, int flags, int fd, off_t offset);
int mprotect(void *addr, size_t length, int prot);
intmunmap(void *addr, size_t length);

// Shared memory functions
int shmget(key_t key, size_t size, int flag);
void *shmat(int shmid, const void *addr, int flag);
int shmctl(int shmid, int cmd, struct shmid_ds *buf);

// Memory allocation constants
#define MAP_FAILED ((void *) -1)
#define MAP_PRIVATE (PROT_READ | PROT_WRITE)
#define MAP_SHARED (PROT_READ | PROT_WRITE | PROT_EXEC)

#endif // MM_H
