#include <stdalign.h>
#include <stdlib.h>
#include "basic.h"

//fixme
#define size ssize

#define anew(a, type, count) arena_put(a, sizeof(type), count, alignof(type))

typedef struct arena {
	size len;
	size capacity;
	byte *data;
} arena;

void arena_init(arena *a, size capacity) {
	a->data     = malloc(capacity);
	a->capacity = capacity;
	a->len      = 0;
	memset(a->data, 0, capacity);
	return;
}

void *arena_put(arena *a, size tsz, size count, size align) {
	size pad = -a->len & (align - 1);
	if (a->len + tsz * count + pad > a->capacity)
		return null;

	a->len += pad;
	void *p = a->data + a->len;
	a->len += tsz * count;

	return memset(p, 0, tsz * count);
}

void arena_reset(arena *a) {
	a->len      = 0;
}

void arena_free(arena *a) {
	free(a->data);
}
