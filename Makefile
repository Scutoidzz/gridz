CC  = gcc
CXX = g++

BUILD_DIR = build
SRC_DIR   = src
KERNEL_DIR = $(SRC_DIR)/kernel
DOOM_DIR  = $(SRC_DIR)/doom/doomgeneric

# ── Flags ──────────────────────────────────────────────────────────────────
# Add include paths for the new structure
COMMON_FLAGS = -I$(DOOM_DIR) \
               -I$(SRC_DIR) \
               -I$(KERNEL_DIR) \
               -I$(KERNEL_DIR)/setup \
               -I$(SRC_DIR)/lib \
               -I$(SRC_DIR)/lib/images \
               -I$(SRC_DIR)/ui \
               -I$(SRC_DIR)/doom \
               -mno-red-zone -mcmodel=kernel \
               -mno-mmx -mno-sse -mno-sse2 \
               -fno-pic -fno-pie -ffreestanding \
               -O2 -Wall -Wextra -fpermissive \
			   -Wunused-variable -Waddress \
			   -fmax-include-depth=4000

CXXFLAGS = $(COMMON_FLAGS) -fno-exceptions -fno-rtti -std=c++17
CFLAGS   = $(COMMON_FLAGS) -std=c11

# ── Source lists ──────────────────────────────────────────────────────────
TARGET    = kernel.elf
DOOM_SRCS = $(wildcard $(DOOM_DIR)/*.c)
KERNEL_C_SRCS = $(KERNEL_DIR)/liballoc.c

CXX_SRCS = $(KERNEL_DIR)/main.cpp \
       $(KERNEL_DIR)/arch/gdt.cpp \
       $(KERNEL_DIR)/allocator.cpp \
       $(KERNEL_DIR)/font.cpp \
       $(KERNEL_DIR)/scheduler.cpp \
       $(KERNEL_DIR)/drivers/ata.cpp \
       $(KERNEL_DIR)/fs/fat32.cpp \
       $(SRC_DIR)/ui/ui.cpp \
       $(SRC_DIR)/ui/components/comp.cpp \
       $(SRC_DIR)/ui/compositor.cpp \
       $(SRC_DIR)/ui/app_manager.cpp \
       $(SRC_DIR)/doom/doom_gridz.cpp \
       $(SRC_DIR)/doom/libc_stubs.cpp \
       $(SRC_DIR)/setup/installer.cpp \
       $(SRC_DIR)/userspace/mainui.cpp

SRCS = $(CXX_SRCS)

ASM_SRCS = $(KERNEL_DIR)/arch/interrupts.asm

# Generate object file paths in the build directory
OBJS = $(patsubst $(SRC_DIR)/%.cpp,$(BUILD_DIR)/%.o,$(CXX_SRCS)) \
       $(patsubst $(SRC_DIR)/%.c,$(BUILD_DIR)/%.o,$(DOOM_SRCS)) \
       $(patsubst $(SRC_DIR)/%.c,$(BUILD_DIR)/%.o,$(KERNEL_C_SRCS)) \
       $(patsubst $(SRC_DIR)/%.asm,$(BUILD_DIR)/%.o,$(ASM_SRCS))

LDFLAGS = -ffreestanding -nostdlib -no-pie -static \
          -Wl,-z,max-page-size=0x1000 -T $(KERNEL_DIR)/linker.ld

# ── Build rules ───────────────────────────────────────────────────────────
all: $(TARGET) sync-apps

sync-apps:

$(TARGET): $(OBJS)
	$(CXX) $(LDFLAGS) -o $@ $^

# Rule for C++ files
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Rule for C files (like Doom)
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

# Rule for Assembly files
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.asm
	@mkdir -p $(dir $@)
	nasm -f elf64 $< -o $@

clean:
	rm -rf $(BUILD_DIR) $(TARGET) $(ISO_IMAGE)

# ── ISO Generation ────────────────────────────────────────────────────────
ISO_IMAGE = gridz.iso
LIMINE_DIR = limine-bin-repo

.PHONY: all clean iso install run dev sync-apps

iso: $(TARGET)
	@if [ ! -d "$(LIMINE_DIR)" ]; then \
		echo "Cloning Limine binaries..."; \
		git clone https://github.com/limine-bootloader/limine.git $(LIMINE_DIR) --branch v7.13.3-binary --depth=1 || \
		(git clone https://github.com/limine-bootloader/limine.git $(LIMINE_DIR) && cd $(LIMINE_DIR) && git checkout v7.13.3-binary); \
	fi
	rm -rf iso_root/limine build/bootblock.bin
	mkdir -p iso_root/boot/limine iso_root/EFI/BOOT build
	cp $(TARGET) iso_root/boot/
	cp src/doom/puredoom/doom1.wad iso_root/DOOM1.WAD 2>/dev/null || true
	# Create bootblock.bin FIRST
	dd if=/dev/zero of=build/bootblock.bin bs=1M count=10 status=none
	python3 -c "\
import struct, sys; \
f = open('build/bootblock.bin', 'r+b'); \
lba_start = 2048; total = 10*1024*1024//512; lba_size = total - lba_start; \
entry = struct.pack('<BBBBBBBBII', 0x80, 0xFE,0xFF,0xFF, 0x0C, 0xFE,0xFF,0xFF, lba_start, lba_size); \
f.seek(446); f.write(entry); f.write(b'\x00'*48); f.write(b'\x55\xAA'); f.close()"
	mformat -i build/bootblock.bin@@1048576 -v "BOOTBLOCK"
	mmd -i build/bootblock.bin@@1048576 ::/boot
	mmd -i build/bootblock.bin@@1048576 ::/boot/limine
	mcopy -i build/bootblock.bin@@1048576 $(TARGET) ::/boot/
	mcopy -i build/bootblock.bin@@1048576 src/doom/puredoom/doom1.wad ::/DOOM1.WAD 2>/dev/null || true
	mcopy -i build/bootblock.bin@@1048576 iso_root/limine.conf ::/boot/limine/
	mcopy -i build/bootblock.bin@@1048576 $(LIMINE_DIR)/limine-bios.sys ::/boot/limine/
	./limine_bin/limine bios-install build/bootblock.bin
	cp build/bootblock.bin iso_root/bootblock.bin
	# Finalize iso_root
	cp iso_root/limine.conf iso_root/limine.cfg
	cp iso_root/limine.conf iso_root/boot/limine/limine.conf
	cp iso_root/limine.conf iso_root/boot/limine/limine.cfg
	cp iso_root/limine.conf iso_root/EFI/BOOT/limine.conf
	cp iso_root/limine.conf iso_root/EFI/BOOT/limine.cfg
	cp $(LIMINE_DIR)/limine-bios.sys $(LIMINE_DIR)/limine-bios-cd.bin $(LIMINE_DIR)/limine-uefi-cd.bin iso_root/boot/limine/
	cp $(LIMINE_DIR)/BOOTX64.EFI $(LIMINE_DIR)/BOOTIA32.EFI iso_root/EFI/BOOT/
	# Create ISO ONCE
	xorriso -as mkisofs -b boot/limine/limine-bios-cd.bin \
		-no-emul-boot -boot-load-size 4 -boot-info-table \
		--efi-boot boot/limine/limine-uefi-cd.bin \
		-efi-boot-part --efi-boot-image --protective-msdos-label \
		iso_root -o $(ISO_IMAGE)
	./limine_bin/limine bios-install $(ISO_IMAGE)

# ── Run in QEMU ───────────────────────────────────────────────────────────
# Development run (using raw directory)
dev: $(TARGET)
	mkdir -p iso_root/boot
	cp $(TARGET) iso_root/boot/
	cp DOOM1.WAD iso_root/ 2>/dev/null || true
	dd if=/dev/zero of=hdd.img bs=1M count=500 status=none
	mformat -i hdd.img -v "Gridz"
	mmd -i hdd.img ::/boot
	qemu-system-x86_64 \
		-M pc \
		-m 512M \
		-bios ./OVMF.fd \
		-drive file=fat:rw:iso_root,format=raw,if=ide \
		-drive file=hdd.img,format=raw,if=ide \
		-serial stdio \
		-no-reboot \
		-display sdl \
		-name "Gridz [Development Mode]"

# INSTALL: Open the Live CD to install the OS
install: $(TARGET) iso
	@if [ ! -f "hdd.img" ]; then dd if=/dev/zero of=hdd.img bs=1M count=500 status=none; fi
	qemu-system-x86_64 \
		-M pc \
		-m 512M \
		-bios ./OVMF.fd \
		-cdrom $(ISO_IMAGE) \
		-drive file=hdd.img,format=raw,if=ide \
		-serial stdio \
		-no-reboot \
		-display sdl \
		-name "Gridz OS [Installer Mode]"
# RUN: Boot directly from the installed hard drive
run: $(TARGET) iso
	@echo "Automating disk deployment..."
	dd if=/dev/zero of=hdd.img bs=1M count=500 status=none
	python3 -c "\
import struct; \
f = open('hdd.img', 'r+b'); \
lba_start = 2048; total = 500*1024*1024//512; lba_size = total - lba_start; \
entry = struct.pack('<BBBBBBBBII', 0x80, 0xFE,0xFF,0xFF, 0xEF, 0xFE,0xFF,0xFF, lba_start, lba_size); \
f.seek(446); f.write(entry); f.write(b'\x00'*48); f.write(b'\x55\xAA'); f.close()"
	mformat -i hdd.img@@1048576 -v "Gridz"
	mmd -i hdd.img@@1048576 ::/limine
	mmd -i hdd.img@@1048576 ::/boot
	mmd -i hdd.img@@1048576 ::/boot/limine
	mmd -i hdd.img@@1048576 ::/EFI
	mmd -i hdd.img@@1048576 ::/EFI/BOOT
	mcopy -i hdd.img@@1048576 $(TARGET) ::/boot/
	mcopy -i hdd.img@@1048576 src/doom/puredoom/doom1.wad ::/DOOM1.WAD 2>/dev/null || true
	mcopy -i hdd.img@@1048576 iso_root/hdd_limine.conf ::/limine.conf
	mcopy -i hdd.img@@1048576 iso_root/hdd_limine.conf ::/limine.cfg
	mcopy -i hdd.img@@1048576 iso_root/hdd_limine.conf ::/limine/limine.conf
	mcopy -i hdd.img@@1048576 iso_root/hdd_limine.conf ::/limine/limine.cfg
	mcopy -i hdd.img@@1048576 iso_root/hdd_limine.conf ::/boot/limine/limine.conf
	mcopy -i hdd.img@@1048576 iso_root/hdd_limine.conf ::/boot/limine/limine.cfg
	mcopy -i hdd.img@@1048576 iso_root/hdd_limine.conf ::/EFI/BOOT/limine.conf
	mcopy -i hdd.img@@1048576 iso_root/hdd_limine.conf ::/EFI/BOOT/limine.cfg
	mcopy -i hdd.img@@1048576 $(LIMINE_DIR)/BOOTX64.EFI ::/EFI/BOOT/
	mcopy -i hdd.img@@1048576 $(LIMINE_DIR)/BOOTIA32.EFI ::/EFI/BOOT/
	mcopy -i hdd.img@@1048576 $(LIMINE_DIR)/limine-bios.sys ::/boot/limine/
	./limine_bin/limine bios-install hdd.img
	qemu-system-x86_64 \
		-M pc \
		-m 512M \
		-bios ./OVMF.fd \
		-drive file=hdd.img,format=raw,if=ide \
		-serial stdio \
		-no-reboot \
		-display sdl \
		-d int,cpu_reset \
		-name "Gridz [Installed]"
