CFLAGS = -I../../libraries -I/usr/lib64/musl/include -ffreestanding -fno-stack-protector -Wall -static -no-pie -g -O3 -nostdlib

LPATH = ../../libraries
LIBS := $(LIBS) $(LPATH)/libc/libc.a

run:
	gcc $(CFLAGS) image_viewer.c -o image.elf $(LIBS)
	mcopy -i ../../../fat16.img -o image.elf ::/image.elf

