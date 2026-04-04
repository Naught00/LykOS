#ifndef STRING_C
#define STRING_C
#include "basic.h"
#include "arena.c"
#include <stdint.h>

#define size ssize

typedef struct { 
	size len;
	char *s;
} string;

string nil_string = {0, ""};

#define string_lit(x) (string){sizeof x - 1, x}
#define string_from_cstring(s) {strlen(s), s};
#define string_stack(x, sz) char s[sz + 1];\
                            x.s = s;\
                            x.len = sz;\
                            memset(x.s, 0, sz + 1);

string string_from_arena(arena *a, size sz) {
	string s;
	s.s = anew(a, char, sz);
	s.len = sz;
	return s;
}

typedef struct { 
	size len;
	size capacity;
	char *s;
} string_builder;

string_builder string_builder_from_arena(arena *a, size sz) {
	string_builder s;

	s.s = anew(a, char, sz);
	s.capacity = sz;
	s.len = 0;
	return s;
}

void string_cat(string_builder *s0, string s1) {
	size offset = 0;
	offset = s0->len + s1.len > s0->capacity 
	         ? s0->capacity - s0->len 
	         : s1.len;
	
	memcpy(s0->s + s0->len, s1.s, offset);
	s0->len += offset;
	return;
}

void string_cat_char(string_builder *s0, char s1) {
	if (s0->len + 1 > s0->capacity) return;
	memcpy(s0->s + s0->len, &s1, 1);
	s0->len += 1;
	return;
}

string string_builder_finish(arena *a, string_builder b) {
	string s;
	s.s = anew(a, char, b.len);
	s.len = b.len;
	memcpy(s.s, b.s, b.len);
	return s;
}

string string_builder_finish_null(string_builder b) {
	string s;
	s.len = b.len;
	s.s   = b.s;
	if (s.len >= b.capacity) s.s[s.len -1] = '\0';
	else s.s[s.len] = '\0';
	return s;
}

void string_builder_reset(string_builder *b) {
	b->len = 0;
	memset(b->s, 0, b->capacity);
	return;
}
#endif
