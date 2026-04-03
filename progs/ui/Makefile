USERSPACELIB = ../../userspace/libraries
USERSPACE    = ../../userspace
CFLAGS = -I$(USERSPACELIB) -I/usr/lib64/musl/include -ffreestanding -fno-stack-protector -Wall -static -no-pie -g -O3 -nostdlib $(USERSPACE)/libraries/libc/crt0.o
all: stb.o
	gcc -std=gnu23 $(CFLAGS) ui.c -o ui.elf stb.o $(USERSPACE)/libraries/libc/libc.a
	mcopy -i ../../fat16.img -o ui.elf ::/ui.elf
stub:
	gcc $(CFLAGS) stbmi.c -o stbmi.elf $(USERSPACE)/libraries/libc/libc.a
	mcopy -i ../../fat16.img -o stbmi.elf ::/stbmi.elf
client:
	gcc $(CFLAGS) client.c -o client.elf $(USERSPACE)/libraries/libc/libc.a
	mcopy -i ../../fat16.img -o client.elf ::/client.elf
stb.o:
	gcc $(CFLAGS) stb.c -c
