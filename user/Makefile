CROSS_COMPILE?=riscv64-unknown-linux-gnu-
CC=$(CROSS_COMPILE)gcc
CXX=$(CROSS_COMPILE)g++
OBJCOPY=$(CROSS_COMPILE)objcopy
CFLAGS=-Wall -Wextra -Wno-attributes -march=rv64gc -mabi=lp64d -ffreestanding -nostdlib -nostartfiles -O0 -g -Ilibc/ -mno-relax -mcmodel=medany -fPIC
LDFLAGS=-Tlds/libc.lds -Llibc
LIBS=-lc
SOURCES=$(wildcard *.c)
OBJECTS=$(patsubst %.c,%.bin,$(SOURCES))

all: clean libc $(OBJECTS)

libc:
	$(MAKE) -C libc

%.bin: %.c Makefile
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@.elf $< $(LIBS)
	$(OBJCOPY) -O binary $@.elf $@


.PHONY: clean libc
	
clean:
	rm -f *.elf *.bin
