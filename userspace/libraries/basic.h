#ifndef BASIC_H
#define BASIC_H
#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <stdbool.h>

typedef uint64_t u64;
typedef int64_t  s64;
typedef uint32_t u32;
typedef int32_t  s32;
typedef uint16_t u16;
typedef int16_t  s16;
typedef uint8_t  u8;
typedef int8_t   s8;
typedef ptrdiff_t ssize;

typedef unsigned char byte;

#define null (void *) 0
#define atomic _Atomic

#define new(type, nmeb) calloc(nmeb, sizeof(type))
#define tcpy(dst, sz, src) memcpy(dst, src, sz * sizeof(*src))
#define memz(x) memset(x, 0, sizeof(*x))

/* Stack */
#define stacktype(t) struct {t *stack; ssize sp;}
#define stacktype2(t, n) struct n {t *stack; ssize sp;}
#define stack(t, sz) struct {t stack[sz]; ssize sp;}
#define push(s, v) s.stack[s.sp++] = v
#define pop(s)  s.stack[--s.sp]
#define popl(s, v) s.sp--
#define top(s) s.stack[s.sp - 1]
#define stack_next(s) s.stack[s.sp++]
#define stack_index(s, i) s.stack[i]
#define modtop(s, v) s.stack[s.sp - 1] = v

/* Strings */
#define streq(a,b) (strcmp((a),(b)) == 0)
#define strstarts(str,prefix) (strncmp((str),(prefix),strlen(prefix)) == 0)

#define size_of(x) (ssize) sizeof(x)
#define countof(x) size_of(x) / size_of(x[0])
#define for_range(lower, upper) for (int it = lower; it < upper; it++)

/* Log */
static inline bool strends(const char *str, const char *postfix)
{
	if (strlen(str) < strlen(postfix))
		return false;

	return streq(str + strlen(str) - strlen(postfix), postfix);
}
#endif
