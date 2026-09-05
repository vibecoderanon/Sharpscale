#---------------------------------------------------------------------------------
# Sharpscale-3DS Configurator Homebrew App Makefile (.3dsx / .cia)
#---------------------------------------------------------------------------------
TARGET := Sharpscale-3DS

CC := arm-none-eabi-gcc
CFLAGS := -Wall -O3 -marm -mcpu=mpcore -mfloat-abi=hard -mtp=soft -D__3DS__ -Iinclude -I../plugin_3gx/include -isystem $(DEVKITPRO)/libctru/include
LDFLAGS := -specs=3dsx.specs -L$(DEVKITPRO)/libctru/lib -lctru

SRCS := $(wildcard src/*.c)
OBJS := $(SRCS:.c=.o)

all: $(TARGET).3dsx

$(TARGET).3dsx: $(TARGET).elf
	3dsxtool $< $@

$(TARGET).elf: $(OBJS)
	$(CC) $(OBJS) $(LDFLAGS) -o $@

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET).elf $(TARGET).3dsx $(TARGET).cia

.PHONY: all clean
