#include "../lykosapi.h"
#include "../../src/vendor/font.h"
#include <ctype.h>
#include "keys.h"

u8 img_buffer[] = {
	#embed "img.png"
};

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

void free2(void *) {
	return;
}
void *realloc2(void *p, size_t sz) {
	u8 *f = p;
	u8 *new = mmap2(sz);
	memcpy(new, f, sz);
	return new;
}

//#include "strcmp.c"
//#include "strncmp.c"
//double pow(double, double) {
//	return 0;
//}
//int abs(int) {
//	return a>0 ? a : -a;
//}
//double ldexp(double, int) {
//	return 0;
//}
//void __assert_fail(const char *, const char *, unsigned int, const char *) {
//	lykos_exit();
//}
void _assert(bool) {
}
//void __isoc23_strtol() {
//}

#include "kalloc.h"
#include "kalloc.c"

#define STBI_NO_STDIO
#define STBI_NO_THREAD_LOCALS
#define STB_IMAGE_IMPLEMENTATION
#define STB_ASSERT(x) _assert(x)
#define STBI_MALLOC kalloc
#define STBI_REALLOC krealloc
#define STBI_FREE kfree
#include "stb_image.h"
stbi__context s;

int main(void) {
	float angle = 3.14;
	float c = cosf(angle);

	int iw, ih, ic;
	u8 *image = stbi_load_from_memory(img_buffer, sizeof img_buffer, &iw, &ih, &ic, 0);
//	stbi__start_mem(&s,img_buffer,sizeof img_buffer);
//	stbi__load_and_postprocess_8bit(&s,&iw,&ih,&ic,0);
	char *f = kalloc(1024);
	kfree(f);
	return 0 ;
}
