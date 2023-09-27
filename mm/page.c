#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "page.h"

// Global variables
static int numPages = 0;
static Page* pages = NULL;

// Function definitions

// Allocate a new page and return a pointer to it
Page* allocatePage() {
    Page* newPage = malloc(sizeof(struct Page));
    newPage->frameNumber = -1;
    newPage->dirty = false;
    newPage->valid = false;
    numPages++;
    pages = realloc(pages, sizeof(Page*) * numPages);
    pages[numPages-1] = newPage;
    return newPage;
}

// Deallocate a page and release its resources
void deallocatePage(Page* page) {
    numPages--;
    page->frameNumber = -1;
    page->dirty = false;
    page->valid = false;
    free(page);
    pages = realloc(pages, sizeof(Page*) * numPages);
}

// Mark a page as dirty
void markPageDirty(Page* page) {
    page->dirty = true;
}

// Mark a page as valid
void markPageValid(Page* page) {
    page->valid = true;
}

// Initialize the page table
void initPageTable() {
    numPages = 0;
    pages = malloc(sizeof(Page*) * 16);
}

// Cleanup the page table
void cleanupPageTable() {
    while (numPages > 0) {
        deallocatePage(pages[numPages-1]);
    }
    free(pages);
}

int main() {
    initPageTable();

    // Create some pages
    Page* page1 = allocatePage();
    Page* page2 = allocatePage();
    Page* page3 = allocatePage();

    // Mark one of the pages as dirty
    markPageDirty(page2);

    // Accessing an invalid page should crash the program
    printf("%p\n", page3->frameNumber);

    // Clean up
    deallocatePage(page1);
    deallocatePage(page2);
    deallocatePage(page3);

    cleanupPageTable();

    return 0;
}
