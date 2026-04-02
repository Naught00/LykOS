CFLAGS = -I../../libraries -I/usr/lib64/musl/include -ffreestanding -fno-stack-protector -Wall -static -no-pie -g -O3 -nostdlib ../../libraries/libc/crt0.o
run:
	gcc $(CFLAGS) command_buffer_example.c -o cbexample.elf ../../libraries/libc/libc.a
	mcopy -i ../../../fat16.img -o cbexample.elf ::/cbexample.elf

