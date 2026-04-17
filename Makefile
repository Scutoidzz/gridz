CXX = g++
DOOM_DIR = src/doom/doomgeneric
CXXFLAGS = -I$(DOOM_DIR) -mno-red-zone -mcmodel=kernel -mno-mmx -mno-sse -mno-sse2 -fno-pic -fno-pie -ffreestanding -O2 -Wall -Wextra -fno-exceptions -fno-rtti -std=c++17
CFLAGS = -I$(DOOM_DIR) -mno-red-zone -mcmodel=kernel -mno-mmx -mno-sse -mno-sse2 -fno-pic -fno-pie -ffreestanding -O2 -Wall -Wextra -std=c11

TARGET = kernel.o
DOOM_SRCS = $(wildcard $(DOOM_DIR)/*.c)
SRCS = src/main.cpp src/font.cpp src/ui/ui.cpp src/doom/doom_gridz.cpp src/doom/libc_stubs.cpp
ASM_SRCS = src/interrups.asm
OBJS = $(SRCS:.cpp=.o) $(DOOM_SRCS:.c=.o) $(ASM_SRCS:.asm=.o)

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) -ffreestanding -nostdlib -r -o $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

%.o: %.asm
	nasm -f elf64 $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)
