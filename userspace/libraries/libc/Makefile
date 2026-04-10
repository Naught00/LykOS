CFLAGS = -I.. -Iinclude -ffreestanding -fno-stack-protector -Wall -static -no-pie -g -O3 -nostdlib -nolibc
all: 
	rm -f libc.a
	nasm -f elf64 -o crt0.o crt0.asm
	gcc $(CFLAGS) libc.c -c
	ar q libc.a  crt0.o libc.o

