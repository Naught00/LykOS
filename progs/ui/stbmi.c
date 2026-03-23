#include "../lykosapi.h"
#include "../../src/vendor/font.h"
#include <ctype.h>
#include "keys.h"

size_t strlen(const char *s) {
	int i;
	while (*s++) i++;
	return i;
}

bool streq(char *s1, char *s2) {
	if (!s1 || !s2) return false;
	while (*s1++ == *s2++)  {
		if (!*s1 && !*s2) return true;
	}
	return false;
}

void *memcpy(void *restrict dest, const void *restrict src, size_t n) {
	u8 *restrict pdest = (u8 *restrict)dest;
	const u8 *restrict psrc = (const u8 *restrict)src;

	for (size_t i = 0; i < n; i++) {
		pdest[i] = psrc[i];
	}

	return dest;
}
void memcpy2(char *src, char *buf, size_t sz) {
	int i;
	for (i = 0; i < sz; i++) {
		src[i] = buf[i];
	}
	return;
}
void *memset(void *s, int c, size_t n) {
	u8 *p = (u8 *)s;

	for (size_t i = 0; i < n; i++) {
		p[i] = (u8)c;
	}

	return s;
}

void free(void *) {
	return;
}
void *realloc(void *p, size_t sz) {
	u8 *f = p;
	u8 *new = mmap(sz);
	memcpy(new, f, sz);
	return new;
}
#include "strcmp.c"
#include "strncmp.c"
double pow(double, double) {
	return 0;
}
int abs(int) {
	return a>0 ? a : -a;
}
double ldexp(double, int) {
	return 0;
}
void __assert_fail(const char *, const char *, unsigned int, const char *) {
	lykos_exit();
}
void _assert(bool) {
}
void __isoc23_strtol() {
}

#define STBI_NO_STDIO
#define STB_IMAGE_IMPLEMENTATION
#define STB_ASSERT(x) _assert(x)
#define STBI_MALLOC mmap
#define STBI_REALLOC realloc
#define STBI_FREE free
#define STBI_ONLY_JPEG
#include "stb_image.h"

int main(void) {
	write("test\n");
	return 0 ;
}
