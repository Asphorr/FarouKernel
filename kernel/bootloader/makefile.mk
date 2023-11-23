CC = gcc
AS = nasm
CFLAGS = -m32 -ffreestanding -nostdlib -nostdinc -fno-builtin -fno-stack-protector -g
ASFLAGS = -f elf

all: boot.bin

boot.bin: boot.o bootloader_main.o bootloader_utils.o
	$(CC) $(CFLAGS) -o $@ $^

boot.o: boot.asm
	$(AS) $(ASFLAGS) -o $@ $<

bootloader_main.o: bootloader_main.c bootloader.h bootloader_utils.h
	$(CC) $(CFLAGS) -c -o $@ $<

bootloader_utils.o: bootloader_utils.c bootloader.h bootloader_utils.h
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f *.o boot.bin
