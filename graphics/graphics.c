#include "graphics.h"

// Functions for drawing shapes
void draw_point(int x, int y) {
    printf("Drawing point at %d, %d\n", x, y);
}

void draw_line(int x1, int y1, int x2, int y2) {
    printf("Drawing line from %d, %d to %d, %d\n", x1, y1, x2, y2);
}

void draw_rectangle(int x, int y, int width, int height) {
    printf("Drawing rectangle at %d, %d with size %d x %d\n", x, y, width, height);
}

void draw_circle(int x, int y, int radius) {
    printf("Drawing circle at %d, %d with radius %d\n", x, y, radius);
}

// Functions for drawing text
void draw_text(const char* text, int x, int y, int font_size) {
    printf("Drawing text '%s' at %d, %d with font size %d\n", text, x, y, font_size);
}

void draw_string(const char* text, int x, int y, int font_size, int length) {
    printf("Drawing string '%s' at %d, %d with font size %d and length %d\n", text, x, y, font_size, length);
}

// Functions for drawing images
void draw_image(const unsigned char* image_data, int x, int y, int width, int height) {
    printf("Drawing image at %d, %d with size %d x %d\n", x, y, width, height);
}

// Functions for setting graphics modes
void set_graphics_mode(int mode) {
    printf("Setting graphics mode to %d\n", mode);
}

// Functions for getting graphics information
int get_screen_width() {
    return 800;
}

int get_screen_height() {
    return 600;
}
