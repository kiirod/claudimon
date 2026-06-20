CC      = gcc
CFLAGS  = -m32 -ffreestanding -fno-stack-protector -fno-pic \
           -mno-red-zone -nostdlib -nostdinc -I include \
           -O2 -Wall -Wextra

NASM      = nasm
NASMFLAGS = -f elf32

LD      = ld
LDFLAGS = -m elf_i386 -T linker.ld --nmagic

BOOT_OBJS = boot/boot.o boot/keyboard_asm.o
C_SRCS    = src/kernel.c src/vga.c src/keyboard.c src/shell.c src/fs.c src/speaker.c src/theme.c src/editor.c src/disk.c
C_OBJS    = $(C_SRCS:.c=.o)
OBJS      = $(BOOT_OBJS) $(C_OBJS)
KERNEL    = claudimon.bin
ISO       = claudimon.iso
DISK      = claudimon_disk.img
DISK_SIZE_MB = 16

all: $(ISO) $(DISK)

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

# Auto-create a blank virtual disk image if it doesn't exist yet.
# This persists between runs (it's a real file on your computer!)
$(DISK):
	dd if=/dev/zero of=$(DISK) bs=1M count=$(DISK_SIZE_MB) status=none

run: $(ISO) $(DISK)
	qemu-system-x86_64 -cdrom $(ISO) -m 32M -hda $(DISK) \
	  -machine pcspk-audiodev=snd -audiodev pa,id=snd 2>/dev/null || \
	qemu-system-x86_64 -cdrom $(ISO) -m 32M -hda $(DISK)

clean:
	rm -f $(BOOT_OBJS) $(C_OBJS) $(KERNEL) $(ISO) iso/boot/$(KERNEL)

# Wipes your saved files too — use if filesystem gets corrupted
clean-disk:
	rm -f $(DISK)

.PHONY: all run clean clean-disk
