CC      = gcc
CFLAGS  = -m32 -ffreestanding -fno-stack-protector -fno-pic \
           -mno-red-zone -nostdlib -nostdinc -I include \
           -O2 -Wall -Wextra

NASM      = nasm
NASMFLAGS = -f elf32

LD      = ld
LDFLAGS = -m elf_i386 -T linker.ld --nmagic

BOOT_OBJS = boot/boot.o boot/keyboard_asm.o
C_SRCS    = src/kernel.c src/vga.c src/keyboard.c src/shell.c
C_OBJS    = $(C_SRCS:.c=.o)
OBJS      = $(BOOT_OBJS) $(C_OBJS)
KERNEL    = mykernel.bin
ISO       = mykernel.iso

all: $(ISO)

boot/boot.o: boot/boot.asm
	$(NASM) $(NASMFLAGS) -o $@ $<

boot/keyboard_asm.o: boot/keyboard_asm.asm
	$(NASM) $(NASMFLAGS) -o $@ $<

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

$(KERNEL): $(OBJS) linker.ld
	$(LD) $(LDFLAGS) -o $@ $(OBJS)

$(ISO): $(KERNEL)
	cp $(KERNEL) iso/boot/$(KERNEL)
	grub2-mkrescue -o $(ISO) iso/

run: $(ISO)
	qemu-system-x86_64 -cdrom $(ISO) -m 32M

clean:
	rm -f $(BOOT_OBJS) $(C_OBJS) $(KERNEL) $(ISO) iso/boot/$(KERNEL)

.PHONY: all run clean
