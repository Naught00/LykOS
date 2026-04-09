CFLAGS = -I../../libraries -I/usr/lib64/musl/include -ffreestanding -fno-stack-protector -Wall -Wextra -static -no-pie -g -O3 -nostdlib 

LPATH = ../../libraries
LIBS := $(LIBS) $(LPATH)/libc/libc.a

all: run
run:
	gcc $(CFLAGS) window_manager.c -o wm.elf $(LIBS)
	mcopy -i ../../../fat16.img -o wm.elf ::/wm.elf
