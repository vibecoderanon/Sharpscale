#---------------------------------------------------------------------------------
# Sharpscale-NX SaltyNX Plugin Makefile
# Builds authentic Nintendo Switch SaltyNX Plugin (.elf) with MOD0 header
#---------------------------------------------------------------------------------
TARGET := sharpscale

CC := aarch64-none-elf-gcc
ARCH := -march=armv8-a -mtune=cortex-a57 -mtp=soft -fPIE
CFLAGS := -Wall -O2 -ffunction-sections -fdata-sections $(ARCH) -Iinclude -isystem $(DEVKITPRO)/libnx/include
LDFLAGS := -z max-page-size=0x1000 -T rel.ld -g -shared -nostdlib -Wl,-Bsymbolic

C_SRCS := $(wildcard src/*.c)
S_SRCS := $(wildcard src/*.s)
OBJS := $(C_SRCS:.c=.o) $(S_SRCS:.s=.o)

all: $(TARGET).elf

$(TARGET).elf: $(OBJS)
	$(CC) $(OBJS) $(LDFLAGS) -o $@

src/%.o: src/%.c
	$(CC) $(CFLAGS) -c $< -o $@

src/%.o: src/%.s
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET).elf

.PHONY: all clean
