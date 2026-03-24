CFLAGS = -I/usr/lib64/musl/include -ffreestanding -fno-stack-protector -Wall -static -no-pie -g -O3 -nostdlib crt0.o
all:
	gcc -std=gnu23 $(CFLAGS) ui.c -o ui.elf /usr/lib64/musl/lib/libc.a 
	mcopy -i ../../fat16.img -o ui.elf ::/ui.elf
stub:
	gcc $(CFLAGS) stbmi.c -o stbmi.elf /usr/lib64/musl/lib/libc.a
	mcopy -i ../../fat16.img -o stbmi.elf ::/stbmi.elf
