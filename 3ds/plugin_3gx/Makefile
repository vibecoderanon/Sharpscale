#---------------------------------------------------------------------------------
# Sharpscale-3DS 3GX Plugin Makefile
#---------------------------------------------------------------------------------
TARGET := sharpscale

CC  := arm-none-eabi-gcc
ARCH := -march=armv6k -mtune=mpcore -mfloat-abi=hard -mtp=soft
CFLAGS := $(ARCH) -Wall -O3 -marm -Iinclude -isystem $(DEVKITPRO)/libctru/include
LDFLAGS := -specs=3dsx.specs $(ARCH) -L$(DEVKITPRO)/libctru/lib -lctru

SRCS := $(wildcard src/*.c)
OBJS := $(SRCS:.c=.o)

all: $(TARGET).3gx

$(TARGET).3gx: $(TARGET).elf
	if command -v 3gxtool >/dev/null 2>&1; then 3gxtool -d $< $@; else cp $< $@; fi

$(TARGET).elf: $(OBJS)
	$(CC) $(OBJS) $(LDFLAGS) -o $@

src/%.o: src/%.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET).elf $(TARGET).3gx

.PHONY: all clean
