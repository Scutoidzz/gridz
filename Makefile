CXX = g++
CXXFLAGS = -mno-red-zone -mcmodel=kernel -mno-mmx -mno-sse -mno-sse2 -fno-pic -fno-pie -ffreestanding -O2 -Wall -Wextra -fno-exceptions -fno-rtti -std=c++17

TARGET = kernel.o
SRCS = src/main.cpp src/font.cpp
OBJS = $(SRCS:.cpp=.o)

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) -ffreestanding -O2 -nostdlib -r -o $@ $^

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)
