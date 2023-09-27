#ifndef FRAME_H
#define FRAME_H

#include <stdint.h>
#include <stdbool.h>

// Structure to represent a frame
struct Frame {
    uint8_t* data;   // Pointer to the frame data
    size_t length;  // Length of the frame data
    bool is_key;   // Flag indicating whether the frame is a key frame
};

// Function prototypes
Frame* allocateFrame(size_t length);
void deallocateFrame(Frame* frame);
void copyFrame(Frame* dest, const Frame* src);
void zeroFillFrame(Frame* frame);

#endif  // FRAME_H
