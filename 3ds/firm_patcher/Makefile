#---------------------------------------------------------------------------------
# Sharpscale-3DS FIRM Matrix Patcher Makefile
# Builds both host CLI binary and 3DS native binary
#---------------------------------------------------------------------------------
TARGET := firm_patcher

CC := gcc
CFLAGS := -Wall -O3 -Iinclude

SRCS := $(wildcard src/*.c)
OBJS := $(SRCS:.c=.o)

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(OBJS) -o $@

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET) $(TARGET).exe

.PHONY: all clean
