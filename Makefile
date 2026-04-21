CC = gcc
CXX = g++
DOOM_DIR = src/doom/doomgeneric
COMMON_FLAGS = -I$(DOOM_DIR) -mno-red-zone -mcmodel=kernel -mno-mmx -mno-sse -mno-sse2 -fno-pic -fno-pie -ffreestanding -O2 -Wall -Wextra -fno-exceptions -fno-rtti
CXXFLAGS = $(COMMON_FLAGS) -std=c++17
CFLAGS = $(COMMON_FLAGS) -std=c11

TARGET = kernel.elf
DOOM_SRCS = $(wildcard $(DOOM_DIR)/*.c)
SRCS = src/main.cpp src/font.cpp src/ui/ui.cpp src/ui/components/comp.cpp src/ui/app_manager.cpp src/doom/doom_gridz.cpp src/doom/libc_stubs.cpp \
       src/drivers/ata.cpp src/fs/fat32.cpp src/userspace/mainui.cpp
ASM_SRCS = src/interrups.asm
OBJS = $(SRCS:.cpp=.o) $(DOOM_SRCS:.c=.o) $(ASM_SRCS:.asm=.o)

LDFLAGS = -ffreestanding -nostdlib -no-pie -static -Wl,-z,max-page-size=0x1000 -T src/linker.ld

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(LDFLAGS) -o $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

%.o: %.asm
	nasm -f elf64 $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)

run: $(TARGET)
	mkdir -p iso_root/boot
	cp $(TARGET) iso_root/boot/
	cp DOOM1.WAD iso_root/ 2>/dev/null || true
	dd if=/dev/zero of=hdd.img bs=1M count=500 status=none
	qemu-system-x86_64 -M q35 -m 512M -bios ./OVMF.fd \
		-drive file=fat:rw:iso_root,format=raw,media=disk \
		-drive file=hdd.img,format=raw,media=disk \
		-serial stdio -no-reboot -net none \
		-display sdl \
		-name "Gridz OS [click window to grab mouse | Ctrl+Alt+G to release]"
