CC = i686-elf-gcc
LD = i686-elf-ld
AS = i686-elf-as

CFLAGS = -Wall -Wextra -Werror -m32 -nostdlib -nostdinc -fno-builtin -fno-stack-protector -nostartfiles -nodefaultlibs -I.
LDFLAGS = -T linker.ld -m elf_i386

all: kernel.bin

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

%.o: %.s
	$(AS) $< -o $@

kernel.bin: boot.o kernel.o
	$(LD) $(LDFLAGS) -o $@ $^

clean:
	rm -f *.o kernel.bin
