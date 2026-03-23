CFLAGS = -msse -msse2 -mno-sse3 -ffreestanding -fno-stack-protector -Wall -static -no-pie -g -O3 -nostdlib ../crt0.c
all:
	gcc $(CFLAGS) ui.c -o ui.elf
	mcopy -i ../../fat16.img -o ui.elf ::/ui.elf
stub:
	gcc $(CFLAGS) stbmi.c -o stbmi.elf
	mcopy -i ../../fat16.img -o stbmi.elf ::/stbmi.elf
