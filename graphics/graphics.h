#ifndef GRAPHICS_H
#define GRAPHICS_H

// Functions for drawing shapes
void draw_point(int x, int y);
void draw_line(int x1, int y1, int x2, int y2);
void draw_rectangle(int x, int y, int width, int height);
void draw_circle(int x, int y, int radius);

// Functions for drawing text
void draw_text(const char* text, int x, int y, int font_size);
void draw_string(const char* text, int x, int y, int font_size, int length);

// Functions for drawing images
void draw_image(const unsigned char* image_data, int x, int y, int width, int height);

// Functions for setting graphics modes
void set_graphics_mode(int mode);

// Functions for getting graphics information
int get_screen_width();
int get_screen_height();

#endif // GRAPHICS_H
