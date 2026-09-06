#---------------------------------------------------------------------------------
# Sharpscale-NX Tesla Overlay Makefile
#---------------------------------------------------------------------------------
TARGET := ovl-sharpscale

CXX := aarch64-none-elf-g++
CXXFLAGS := -Wall -O3 -std=gnu++17 -fPIC -fvisibility=hidden -Iinclude -I../plugin/include -isystem $(DEVKITPRO)/libnx/include -isystem $(DEVKITPRO)/libtesla/include
LDFLAGS := -shared -nostdlib -Wl,-Bsymbolic -L$(DEVKITPRO)/libnx/lib -lnx

SRCS := $(wildcard src/*.cpp) ../plugin/src/config.c
OBJS := $(SRCS:.cpp=.o)
OBJS := $(OBJS:.c=.o)

all: $(TARGET).ovl

$(TARGET).ovl: $(OBJS)
	$(CXX) $(OBJS) $(LDFLAGS) -o $@

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

%.o: %.c
	aarch64-none-elf-gcc $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET).ovl

.PHONY: all clean
