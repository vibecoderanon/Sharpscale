#---------------------------------------------------------------------------------
# Sharpscale-NX SaltyNX Plugin Makefile
#---------------------------------------------------------------------------------
TARGET := sharpscale

CC := aarch64-none-elf-gcc
CFLAGS := -Wall -O3 -fPIC -fvisibility=hidden -Iinclude -isystem $(DEVKITPRO)/libnx/include
LDFLAGS := -shared -nostdlib -Wl,-Bsymbolic -L$(DEVKITPRO)/libnx/lib -lnx

SRCS := $(wildcard src/*.c)
OBJS := $(SRCS:.c=.o)

all: $(TARGET).elf

$(TARGET).elf: $(OBJS)
	$(CC) $(OBJS) $(LDFLAGS) -o $@

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET).elf

.PHONY: all clean
