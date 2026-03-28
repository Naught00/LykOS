CFLAGS = -I/usr/lib64/musl/include -ffreestanding -fno-stack-protector -Wall -static -no-pie -g -O3 -nostdlib ../crt0.o
run:
	gcc $(CFLAGS) wm_test.c -o wm_client /home/naught/git/musl/lib/libc.a
	mcopy -i ../../../fat16.img -o wm_client ::/wmclient.elf

