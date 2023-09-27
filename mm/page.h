#ifndef PAGE_H
#define PAGE_H

#include <stdint.h>
#include <stdbool.h>

// Structure to represent a page in the pagetable
typedef struct Page {
    uint32_t frameNumber; // Frame number where this page is stored
    bool dirty;         // Whether or not this page has been modified
    bool valid;         // Whether or not this page is currently in use
} Page;

// Function prototypes
Page* allocatePage();
void deallocatePage(Page* page);
void markPageDirty(Page* page);
void markPageValid(Page* page);

#endif  // PAGE_H
