#ifndef BOOTLOADER_H
#define BOOTLOADER_H

extern void start();
extern void load_kernel();
extern void switch_to_pm();
extern void start_protected_mode();

#endif
