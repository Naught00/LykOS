NAME = gol
CFLAGS = -I../../ -I../../libraries -I/usr/lib64/musl/include -ffreestanding -fno-stack-protector -Wall -static -no-pie -g -O3 -nostdlib ../../libraries/libc/crt0.o
run:
	gcc $(CFLAGS) gol.c -o gol.elf ../../libraries/libc/libc.a
	mcopy -i ../../../fat16.img -o gol.elf ::/gol.elf

