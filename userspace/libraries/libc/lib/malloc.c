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

static header *first_fit(size_t sz) {
	int i;
	for (i = 0; i < freelist.sp; i++) {
		header *h = freelist.stack[i];
		if (h->capacity >= sz) {
			stack_remove_index(freelist, i);
			return h;
		}
	}
	return null;
}

void *malloc(size_t sz) {
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
	header *h = (header *) p - 1;
	if (freelist.sp < countof(freelist.stack)) {
		memset(h->data, 0, h->capacity);
		push(freelist, h);
	} else {
		//kernel free;
	}
}

void *realloc(void *p, size_t sz) {
	header *old = (header *) p - 1;
	void *newp = malloc(sz);
	header *new = (header *) newp - 1;

	memmove(new->data, old->data, old->capacity);
	return new->data;
}
