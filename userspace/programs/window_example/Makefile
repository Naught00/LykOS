CFLAGS = -I../../libraries -I/usr/lib64/musl/include -ffreestanding -fno-stack-protector -Wall -static -no-pie -g -O3 -nostdlib ../../libraries/libc/crt0.o
run:
	gcc $(CFLAGS) window_example.c -o wexample.elf ../../libraries/libc/libc.a
	mcopy -i ../../../fat16.img -o wexample.elf ::/wexample.elf

