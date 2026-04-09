USERSPACELIB = ../../userspace/libraries
USERSPACE    = ../../userspace
CFLAGS = -I$(USERSPACELIB) -I/usr/lib64/musl/include -ffreestanding -fno-stack-protector -Wall -static -no-pie -g -O3 -nostdlib
all: run

LPATH = $(USERSPACELIB)
LIBS := $(LIBS) stb.o
LIBS := $(LIBS) $(LPATH)/libc/libc.a
LIBS := $(LIBS) $(LPATH)/libc/musl.a

run: stb.o
	gcc -std=gnu23 $(CFLAGS) ui.c -o ui.elf $(LIBS)
	mcopy -i ../../fat16.img -o ui.elf ::/ui.elf
stub:
	gcc $(CFLAGS) stbmi.c -o stbmi.elf $(USERSPACE)/libraries/libc/libc.a
	mcopy -i ../../fat16.img -o stbmi.elf ::/stbmi.elf
client:
	gcc $(CFLAGS) client.c -o client.elf $(USERSPACE)/libraries/libc/libc.a
	mcopy -i ../../fat16.img -o client.elf ::/client.elf
stb.o: stb.c
	gcc $(CFLAGS) stb.c -c
