#---------------------------------------------------------------------------------
# Sharpscale-3DS 3GX Plugin Makefile
#---------------------------------------------------------------------------------
TARGET := sharpscale

CC := arm-none-eabi-gcc
CFLAGS := -Wall -O3 -marm -mcpu=mpcore -mfloat-abi=hard -mtp=soft -Iinclude -isystem $(DEVKITPRO)/libctru/include
LDFLAGS := -specs=3dsx.specs -L$(DEVKITPRO)/libctru/lib -lctru

SRCS := $(wildcard src/*.c)
OBJS := $(SRCS:.c=.o)

all: $(TARGET).3gx

$(TARGET).3gx: $(OBJS)
	$(CC) $(OBJS) $(LDFLAGS) -o $(TARGET).elf
	3gxtool -d $(TARGET).elf $@

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET).elf $(TARGET).3gx

.PHONY: all clean
