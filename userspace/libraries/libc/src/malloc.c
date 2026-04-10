#pragma once
#include "basic.h"
#include "lykosapi.h"
#include <stdint.h>
#include <string.h>
#include <stdlib.h>


typedef struct header header;
struct header {
	size_t len;
	size_t capacity;
	uint8_t data[];
};

stack(header *, 20) freelist;

static u8 valid_mem[1];

static header *first_fit(size_t sz) {
	int i;
	for (i = 0; i < freelist.sp; i++) {
		header *h = freelist.stack[i];
		if (h->capacity == sz) {
			int j;
			for (j = i; j < freelist.sp - 1; j++) {
				freelist.stack[j] = freelist.stack[j + 1];
			}
			freelist.stack[j] = null;
			freelist.sp--;
			return h;
		} else if (h->capacity > sz + sizeof(header)) {
			int leftover = h->capacity - (sz + sizeof(header));
			header *new_header = (void *) (h->data + leftover);

			new_header->capacity = sz;
			h->capacity = leftover;
			return new_header;
		}
	}
	return null;
}

void *malloc(size_t sz) {
	if (!sz) return valid_mem;
	header *h;
	if (freelist.sp) {
		h = first_fit(sz);
		if (h) return h->data;
	}

	h = mmap(sizeof(header) + sz);
	h->len = 0;
	h->capacity = sz;
	memset(h->data, 0, h->capacity);
	return h->data;

}

void *calloc(size_t n, size_t sz) {
	return malloc(n * sz);
}

void free(void *p) {
	if (!p) return;
	if (p == valid_mem) return;
	header *h = (header *) p - 1;
	if (freelist.sp < countof(freelist.stack)) {
		memset(h->data, 0, h->capacity);
		push(freelist, h);
	} else {
		//kernel free;
	}
}

void *realloc(void *p, size_t sz) {
	if (!p) return malloc(sz);
	if (p && !sz) {
		free(p);
		return null;
	}
	header *old = (header *) p - 1;
	void *newp = malloc(sz);
	memmove(newp, p, old->capacity);
	return newp;
}
