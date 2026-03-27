CFLAGS = -I/usr/lib64/musl/include -ffreestanding -fno-stack-protector -Wall -static -no-pie -g -O0 -nostdlib crt0.o
all: client
	gcc -std=gnu23 $(CFLAGS) ui.c -o ui.elf stb.o /home/naught/git/musl/lib/libc.a
	mcopy -i ../../fat16.img -o ui.elf ::/ui.elf
stub:
	gcc $(CFLAGS) stbmi.c -o stbmi.elf /usr/lib64/musl/lib/libc.a
	mcopy -i ../../fat16.img -o stbmi.elf ::/stbmi.elf
client:
	gcc $(CFLAGS) client.c -o client.elf /usr/lib64/musl/lib/libc.a
	mcopy -i ../../fat16.img -o client.elf ::/client.elf
stb:
	gcc $(CFLAGS) stb.c -c
