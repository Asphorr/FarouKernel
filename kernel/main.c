#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <boot.h>
#include <kernel.h>
#include <console.h>
#include <drivers/keyboard.h>
#include <drivers/display.h>

// Define our kernel's entry point
extern void kernel_entry(void);

// Define our kernel's name
const char kernel_name[] = "MyFirstKernel";

// Define our kernel's version number
const uint16_t kernel_version = 0x0100;

// Define our kernel's release date
const char kernel_release_date[] = "2023-09-28";

// Define our kernel's build time
const char kernel_build_time[] = "14:30:00";

// Define our kernel's author
const char kernel_author[] = "Your Name";

// Define our kernel's copyright information
const char kernel_copyright[] = "Copyright (C) 2023 Mikhail";

// Define our kernel's license information
const char kernel_license[] = "Licensed under the MIT License";

// Define our kernel's description
const char kernel_description[] = "A simple kernel for learning purposes.";

// Define our kernel's logo
const char kernel_logo[] = "\n\t _______ \n\t|       | \n\t|   o   | \n\t|  ---  | \n\t|________| \n";

// Print our kernel's logo and identification information
static inline void print_kernel_info() {
    puts(kernel_logo);
    printf("Kernel: %s\n", kernel_name);
    printf("Version: %d.%d\n", kernel_version >> 8, kernel_version & 0xFF);
    printf("Release Date: %s\n", kernel_release_date);
    printf("Build Time: %s\n", kernel_build_time);
    printf("Author: %s\n", kernel_author);
    printf("Copyright: %s\n", kernel_copyright);
    printf("License: %s\n", kernel_license);
    printf("Description: %s\n", kernel_description);
}

// Handle keyboard input
static inline void handle_keyboard(struct keyboard_event *event) {
    if (event->keycode == KEYCODE_ESCAPE) {
        printf("\nEscape key pressed. Shutting down...\n");
        shutdown();
    }
}

// Handle display output
static inline void handle_display(struct display_event *event) {
    const char *buffer = event->data;
    size_t len = event->len;

    // Print the display data to the console
    fputs(buffer, stdout);
}

// Main entry point
int main(void) {
    // Initialize the keyboard driver
    init_keyboard();

    // Initialize the display driver
    init_display();

    // Register our keyboard and display handlers
    register_keyboard_handler(handle_keyboard);
    register_display_handler(handle_display);

    // Enable keyboard and display events
    enable_keyboard_events();
    enable_display_events();

    // Print our kernel's logo and identification information
    print_kernel_info();

    // Enter an infinite loop
    while (1) {
        // Handle any pending events
        handle_events();
    }

    return 0;
}

// Entry point for our kernel
void kernel_entry(void) {
    // Call our main function
    main();
}
