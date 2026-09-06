#---------------------------------------------------------------------------------
# Sharpscale-NX Tesla Overlay Makefile
# Builds authentic Nintendo Switch Tesla Overlay (.ovl)
#---------------------------------------------------------------------------------
TARGET := ovl-sharpscale

CC  := aarch64-none-elf-gcc
CXX := aarch64-none-elf-g++

ARCH := -march=armv8-a+crc+crypto -mtune=cortex-a57 -mtp=soft -fPIE
COMMON_FLAGS := -Wall -O2 -ffunction-sections -fdata-sections $(ARCH) -D__SWITCH__ -Iinclude -I../plugin/include -isystem $(DEVKITPRO)/libnx/include -isystem $(DEVKITPRO)/libtesla/include
CFLAGS := $(COMMON_FLAGS) -std=gnu17
CXXFLAGS := $(COMMON_FLAGS) -fno-exceptions -std=gnu++20
LDFLAGS := -specs=$(DEVKITPRO)/libnx/switch.specs -g $(ARCH) -Wl,--gc-sections -L$(DEVKITPRO)/libnx/lib -lnx

OBJS := src/main.o config.o

all: $(TARGET).ovl

$(TARGET).ovl: $(TARGET).elf $(TARGET).nacp
	elf2nro $< $@ --nacp=$(TARGET).nacp
	@echo "Built $(TARGET).ovl successfully."

$(TARGET).nacp:
	nacptool --create "Sharpscale-NX" "vibecoderanon" "1.0.0" $@

$(TARGET).elf: $(OBJS)
	$(CXX) $(OBJS) $(LDFLAGS) -o $@

src/main.o: src/main.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

config.o: ../plugin/src/config.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET).elf $(TARGET).ovl $(TARGET).nacp

.PHONY: all clean
