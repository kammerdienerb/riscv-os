CROSS_COMPILE?=riscv64-unknown-linux-gnu-
GDB=gdb-multiarch
CC=$(CROSS_COMPILE)gcc
CXX=$(CROSS_COMPILE)g++
OBJCOPY=$(CROSS_COMPILE)objcopy
CFLAGS=-g -O0 -Wall -Wextra -Wno-attributes -Wno-unused-parameter -march=rv64gc -mabi=lp64d -ffreestanding -nostdlib -nostartfiles -Isrc/include -mcmodel=medany
LDFLAGS=-Tlds/riscv.lds $(CFLAGS)
LIBS=
ASM_DIR=asm
SOURCE_DIR=src
OBJ_DIR=objs
DEP_DIR=deps
SOURCES=$(wildcard $(SOURCE_DIR)/*.c)
SOURCES_ASM=$(wildcard $(ASM_DIR)/*.S)
OBJECTS=$(patsubst $(SOURCE_DIR)/%.c,$(OBJ_DIR)/%.o,$(SOURCES))
OBJECTS+= $(patsubst $(ASM_DIR)/%.S,$(OBJ_DIR)/%.o,$(SOURCES_ASM))
DEPS=$(patsubst $(SOURCE_DIR)/%.c,$(DEP_DIR)/%.d,$(SOURCES))
KERNEL=cosc562.elf

#### QEMU STUFF
QEMU?=qemu-system-riscv64
QEMU_DEBUG_PIPE=debug.pipe
QEMU_HARD_DRIVE_1=hdd1.dsk
QEMU_HARD_DRIVE_2=hdd2.dsk
QEMU_BIOS=./sbi/sbi.elf
QEMU_DEBUG=guest_errors,unimp
QEMU_MACH=virt
QEMU_CPU=rv64
QEMU_CPUS=8
QEMU_MEM=256M
QEMU_KERNEL=$(KERNEL)
QEMU_OPTIONS= -serial mon:stdio -gdb unix:$(QEMU_DEBUG_PIPE),server,nowait
QEMU_DEVICES+= -device pcie-root-port,id=rp1,multifunction=off,chassis=0,slot=1,bus=pcie.0,addr=01.0
QEMU_DEVICES+= -device pcie-root-port,id=rp2,multifunction=off,chassis=1,slot=2,bus=pcie.0,addr=02.0
QEMU_DEVICES+= -device pcie-root-port,id=rp3,multifunction=off,chassis=2,slot=3,bus=pcie.0,addr=03.0
QEMU_DEVICES+= -device pcie-root-port,id=rp4,multifunction=off,chassis=3,slot=4,bus=pcie.0,addr=04.0
QEMU_DEVICES+= -device virtio-keyboard-pci,bus=rp1,id=keyboard
QEMU_DEVICES+= -device virtio-tablet,bus=rp1,id=tablet
QEMU_DEVICES+= -device virtio-gpu-pci,bus=rp2,id=gpu
QEMU_DEVICES+= -device virtio-rng-pci-non-transitional,bus=rp1,id=rng
QEMU_DEVICES+= -device virtio-blk-pci-non-transitional,drive=hdd1,bus=rp2,id=blk1
QEMU_DEVICES+= -device virtio-blk-pci-non-transitional,drive=hdd2,bus=rp2,id=blk2
QEMU_DEVICES+= -device qemu-xhci,bus=rp3,id=usbhost
QEMU_DEVICES+= -drive if=none,format=raw,file=$(QEMU_HARD_DRIVE_1),id=hdd1
QEMU_DEVICES+= -drive if=none,format=raw,file=$(QEMU_HARD_DRIVE_2),id=hdd2

all: clean sbi user $(KERNEL)

include $(wildcard $(DEP_DIR)/*.d)

run: clean sbi user $(KERNEL)
	$(QEMU) -nographic -bios $(QEMU_BIOS) -d $(QEMU_DEBUG) -cpu $(QEMU_CPU) -machine $(QEMU_MACH) -smp $(QEMU_CPUS) -m $(QEMU_MEM) -kernel $(QEMU_KERNEL) $(QEMU_OPTIONS) $(QEMU_DEVICES)

rungui: clean sbi user $(KERNEL)
	$(QEMU) -bios $(QEMU_BIOS) -d $(QEMU_DEBUG) -cpu $(QEMU_CPU) -machine $(QEMU_MACH) -smp $(QEMU_CPUS) -m $(QEMU_MEM) -kernel $(QEMU_KERNEL) $(QEMU_OPTIONS) $(QEMU_DEVICES)

$(KERNEL): sbi $(OBJECTS) lds/riscv.lds
	$(CC) $(LDFLAGS) -o $@ $(OBJECTS) $(LIBS)

sbi:
	$(MAKE) -C sbi

user:
	$(MAKE) -C user
	./copy_programs.sh

$(OBJ_DIR)/%.o: $(SOURCE_DIR)/%.c Makefile
	$(CC) -MD -MF ./deps/$*.d $(CFLAGS) -o $@ -c $<

$(OBJ_DIR)/%.o: $(ASM_DIR)/%.S Makefile
	$(CC) $(CFLAGS) -o $@ -c $<



.PHONY: sbi clean gdb run rungui hdd user

hdd:
	fallocate -l 32M hdd.dsk
	/sbin/mkfs.ext4 -L MYNEWFS hdd.dsk

gdb: $(KERNEL)
	$(GDB) $< -ex "target extended-remote $(QEMU_DEBUG_PIPE)"
	rm -f $(QEMU_DEBUG_PIPE)

clean:
	$(MAKE) -C sbi clean
	rm -f $(OBJ_DIR)/*.o $(DEP_DIR)/*.d $(KERNEL)
