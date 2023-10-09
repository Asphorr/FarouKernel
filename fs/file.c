#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>

// Define constants for error handling
enum ErrorCodes {
    ERROR_OPENING_FILE = 1,
    ERROR_READING_FROM_FILE,
    ERROR_WRITING_TO_FILE,
    ERROR_LSEEKING_IN_FILE,
    ERROR_STATING_FILE,
    ERROR_FSTATING_FD,
    ERROR_MALLOC_FAILED,
    ERROR_REALLOC_FAILED,
    ERROR_MMAP_FAILED,
    ERROR_UNEXPECTED_ERROR,
};

// Structure to represent a file
typedef struct File {
    // Name of the file
    char name[MAX_NAME_LEN];
    
    // File descriptor
    int fd;
    
    // Size of the file
    off_t size;
    
    // Pointer to the memory-mapped region
    void *map;
    
    // Offset within the mapped region
    off_t mapOffset;
    
    // Length of the mapped region
    size_t mapLength;
} File;

// Function prototypes
void printErrorMessage(int errCode);
File *createFileStruct();
void destroyFileStruct(File **filePtr);
int openFile(File *file, const char *fileName, int flags);
off_t getFileSize(File *file);
ssize_t readFromFile(File *file, void *buffer, size_t nbytes);
ssize_t writeToFile(File *file, const void *buffer, size_t nbytes);
off_t seekInFile(File *file, off_t offset, int whence);
int mmapFile(File *file, void *addr, size_t length, int prot, int flags, off_t offset);
void unmapFile(File *file);

int main() {
    // Create a new file structure
    File *file = createFileStruct();
    
    // Open an existing file or create a new one
    int result = openFile(file, "example.txt", O_CREAT | O_RDWR);
    if (result != 0) {
        printErrorMessage(result);
        goto cleanup;
    }
    
    // Get the size of the file
    off_t fileSize = getFileSize(file);
    printf("File size: %ld\n", fileSize);
    
    // Read from the file
    char buffer[1024];
    ssize_t numBytesRead = readFromFile(file, buffer, sizeof(buffer));
    if (numBytesRead > 0) {
        printf("Read %ld bytes\n", numBytesRead);
        
        // Write back to the file
        ssize_t numBytesWritten = writeToFile(file, buffer, numBytesRead);
        if (numBytesWritten >= 0) {
            printf("Wrote %ld bytes\n", numBytesWritten);
            
            // Seek to the beginning of the file
            off_t currentPos = seekInFile(file, 0, SEEK_SET);
            if (currentPos >= 0) {
                printf("Current position: %ld\n", currentPos);
                
                // Memory-map the file
                void *mapAddr = NULL;
                size_t mapLen = fileSize;
                int mapProt = PROT_READ | PROT_WRITE;
                int mapFlags = MAP_PRIVATE;
                off_t mapOff = 0;
                result = mmapFile(file, &mapAddr, mapLen, mapProt, mapFlags, mapOff);
                if (result != 0) {
                    printErrorMessage(result);
                    goto cleanup;
                }
                
                // Access the memory-mapped data
                memcpy(buffer, mapAddr + mapOff, numBytesRead);
                printf("Mapped data: ");
                for (int i = 0; i < numBytesRead; ++i) {
                    printf("%c", buffer[i]);
                }
                putchar('\n');
                
                // Unmap the file
                unmapFile(file);
            }
        }
    }
    
cleanup:
    // Close the file
    close(file->fd);
    
    // Free the file structure
    destroyFileStruct(&file);
    
    return 0;
}

// Print an error message based on the given error code
void printErrorMessage(int errCode) {
    switch (errCode) {
        case ERROR_OPENING_FILE:
            puts("Failed to open file.");
            break;
        case ERROR_READING_FROM_FILE:
            puts("Failed to read from file.");
            break;
        case ERROR_WRITING_TO_FILE:
            puts("Failed to write to file.");
            break;
        case ERROR_LSEEKING_IN_FILE:
            puts("Failed to seek in file.");
            break;
        case ERROR_STATING_FILE:
            puts("Failed to stat file.");
            break;
        case ERROR_FSTATING_FD:
            puts("Failed to fstat file descriptor.");
            break;
        case ERROR_MALLOC_FAILED:
            puts("Memory allocation failed.");
            break;
        case ERROR_REALLOC_FAILED:
            puts("Reallocating memory failed.");
            break;
        case ERROR_MMAP_FAILED:
            puts("Mapping memory failed.");
            break;
        default:
            puts("An unexpected error occurred.");
            break;
    }
}

// Allocate and initialize a new file structure
File *createFileStruct() {
    File *file = malloc(sizeof(*file));
    if (!file) {
        printErrorMessage(ERROR_MALLOC_FAILED);
        exit(-1);
    }
    memset(file, 0, sizeof(*file));
    return file;
}

// Deallocate a file structure
void destroyFileStruct(File **filePtr) {
    if (*filePtr) {
        free(*filePtr);
        *filePtr = NULL;
    }
}

// Open a file using the specified mode
int openFile(File *file, const char *fileName, int flags) {
    int fd = open(fileName, flags);
    if (fd == -1) {
        printErrorMessage(ERROR_OPENING_FILE);
        return -1;
    }
    file->fd = fd;
    return 0;
}

// Get the size of a file
off_t getFileSize(File *file) {
    struct stat st;
    if (fstat(file->fd, &st) == -1) {
        printErrorMessage(ERROR_FSTATING_FD);
        return -1;
    }
    return st.st_size;
}

// Read from a file into a buffer
ssize_t readFromFile(File *file, void *buffer, size_t nbytes) {
    ssize_t numBytesRead = read(file->fd, buffer, nbytes);
    if (numBytesRead <= 0) {
        printErrorMessage(ERROR_READING_FROM_FILE);
        return -1;
}
return numBytesRead;
}

// Write to a file from a buffer
ssize_t writeToFile(File *file, const void *buffer, size_t nbytes) {
ssize_t numBytesWritten = write(file->fd, buffer, nbytes);
if (numBytesWritten <= 0) {
printErrorMessage(ERROR_WRITING_TO_FILE);
return -1;
}
return numBytesWritten;
}

// Seek within a file
off_t lseekInFile(File *file, off_t offset, int whence) {
off_t currOffset = lseek(file->fd, offset, whence);
if (currOffset == -1) {
printErrorMessage(ERROR_LSEEKING_IN_FILE);
return -1;
}
return currOffset;
}
