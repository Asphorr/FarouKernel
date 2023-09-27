#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "frame.h"

// Global variables
static int numFrames = 0;
static Frame* frames = NULL;

// Function definitions

// Allocate a new frame and return a pointer to it
Frame* allocateFrame() {
    Frame* newFrame = malloc(sizeof(struct Frame));
    newFrame->number = numFrames++;
    newFrame->prev = NULL;
    newFrame->next = NULL;
    newFrame->data = NULL;
    return newFrame;
}

// Deallocate a frame and release its resources
void deallocateFrame(Frame* frame) {
    if (frame == NULL) {
        return;
    }
    numFrames--;
    if (frame->prev != NULL) {
        frame->prev->next = frame->next;
    } else {
        frames = frame->next;
    }
    if (frame->next != NULL) {
        frame->next->prev = frame->prev;
    }
    free(frame);
}

// Set the data associated with a frame
void setFrameData(Frame* frame, void* data) {
    frame->data = data;
}

// Get the data associated with a frame
void* getFrameData(Frame* frame) {
    return frame->data;
}

// Insert a frame into the list of frames
void insertFrame(Frame* frame) {
    if (frames == NULL) {
        frames = frame;
    } else {
        Frame* current = frames;
        while (current->next != NULL) {
            current = current->next;
        }
        current->next = frame;
        frame->prev = current;
    }
}

// Remove a frame from the list of frames
void removeFrame(Frame* frame) {
    if (frame == NULL) {
        return;
    }
    if (frame->prev == NULL) {
        frames = frame->next;
    } else {
        frame->prev->next = frame->next;
    }
    if (frame->next == NULL) {
        frame->prev->next = NULL;
    }
    free(frame);
}

int main() {
    // Test cases
    Frame* frame1 = allocateFrame();
    Frame* frame2 = allocateFrame();
    Frame* frame3 = allocateFrame();

    setFrameData(frame1, (void*) 1);
    setFrameData(frame2, (void*) 2);
    setFrameData(frame3, (void*) 3);

    insertFrame(frame1);
    insertFrame(frame2);
    insertFrame(frame3);

    void* data1 = getFrameData(frame1);
    void* data2 = getFrameData(frame2);
    void* data3 = getFrameData(frame3);

    printf("Data in frame 1: %d\n", (int) data1);
    printf("Data in frame 2: %d\n", (int) data2);
    printf("Data in frame 3: %d\n", (int) data3);

    removeFrame(frame2);

    data1 = getFrameData(frame1);
    data2 = getFrameData(frame3);

    printf("Data in frame 1 after removal: %d\n", (int) data1);
    printf("Data in frame 3 after removal: %d\n", (int) data2);

    return 0;
}
