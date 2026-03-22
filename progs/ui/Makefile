CFLAGS = -ffreestanding -Wall -static -no-pie -O3 -nostdlib ../crt0.c
all:
	gcc $(CFLAGS) ui.c -o ui.elf
	mcopy -i ../../fat16.img -o ui.elf ::/ui.elf
