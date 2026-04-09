CFLAGS = -I../../libraries -I/usr/lib64/musl/include -ffreestanding -fno-stack-protector -Wall -static -no-pie -g -O3 -nostdlib

LPATH = ../../libraries
LIBS := $(LIBS) $(LPATH)/libc/libc.a
LIBS := $(LIBS) $(LPATH)/libc/musl.a

run:
	gcc $(CFLAGS) window_example.c -o wexample.elf $(LIBS)
	mcopy -i ../../../fat16.img -o wexample.elf ::/wexample.elf

