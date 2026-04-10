# compiles all programs in progs/ and moves them to the filesystem
#
CC = gcc
CFLAGS = -ffreestanding -Wall -static -no-pie -O3 -fno-stack-protector
#CFLAGS = -ffreestanding -Wall -static -no-pie -O3 -nostartfiles -nostdlib -fno-stack-protector
# CFLAGS = -ffreestanding -Wall -static -no-pie -O3 -mno-sse -mno-sse2 
LDFLAGS = -nostdlib crt0.o
FAT_IMG = ../fat16.img

SRCS = $(wildcard *.c)
SRCS := $(filter-out crt0.c, $(SRCS))
ELFS = $(SRCS:.c=.elf)

all: crt0.o $(ELFS) install

crt0.o: crt0.c
	$(CC) $(CFLAGS) -c -o crt0.o crt0.c


%.elf: %.o crt0.o
	ld -nostdlib -static -o $@ crt0.o $<

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

install: $(ELFS)
	@for f in $(ELFS); do \
		echo "MCOPY $$f"; \
		mcopy -i $(FAT_IMG) -o $$f ::/$$f; \
	done
	rm -f *.o


clean:
	rm -f *.o
	# rm -f *.elf *.o

.PHONY: all install clean
