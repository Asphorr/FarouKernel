#include <stdio.h>
#include <stdlib.h>

// Define a structure for the system
typedef struct {
    int system_id;
    char system_name[20];
} System;

// Function to initialize the system
void system_init(System *system, int id, const char *name) {
    system->system_id = id;
    strcpy(system->system_name, name);
}

// Function to print system information
void system_print(System *system) {
    printf("System ID: %d\n", system->system_id);
    printf("System Name: %s\n", system->system_name);
}

int main() {
    // Create a system
    System system;

    // Initialize the system
    system_init(&system, 1, "My System");

    // Print the system information
    system_print(&system);

    return 0;
}
