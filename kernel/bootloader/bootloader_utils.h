#ifndef BOOTLOADER_UTILS_H
#define BOOTLOADER_UTILS_H

#include "bootloader_config.h"

void print_string(char* str);
void print_hex(int num);
void disk_load(char* buffer, int sector, int head, int cylinder, int drive);

#endif
