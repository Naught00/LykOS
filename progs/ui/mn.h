#ifndef GEN_H
#define GEN_H
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
typedef ptrdiff_t size;

typedef unsigned char byte;
/* 4 shorts = quad */
typedef long long quad;

#define null (void *) 0
#define atomic _Atomic

#ifdef CCAN_ENDIAN_H
 /* 64/32/16 bit big-endian representation. */
typedef beint64_t be64;
typedef beint32_t be32;
typedef beint16_t be16;

 /* le64/le32/le16 - 64/32/16 bit little-endian representation. */
typedef leint64_t le64;
typedef leint32_t le32;
typedef leint16_t le16;
#endif

#define new(type, nmeb) calloc(nmeb, sizeof(type))
#define tcpy(dst, sz, src) memcpy(dst, src, sz * sizeof(*src))

void memcat(uint8_t *output, uint8_t *p1, size_t s1, uint8_t *p2, size_t s2);
void memcatv(void *output, size_t *sizes, int sizec, ...);

void _print_errno(int line, char *file);
#define print_errno() _print_errno(__LINE__, __FILE__);

#define NUMARGS(...)  (sizeof((char *[]){__VA_ARGS__})/sizeof(char *))

void free_all(int argc, ...);

#define memz(x) memset(x, 0, sizeof(*x))

/* Stack */
#define stacktype(t) struct {t *stack; size sp;}
#define stacktype2(t, n) struct n {t *stack; size sp;}
#define stack(t, sz) struct {t stack[sz]; size sp;}
#define push(s, v) s.stack[s.sp++] = v
#define pop(s)  s.stack[--s.sp]
#define popl(s, v) s.sp--
#define top(s) s.stack[s.sp - 1]
#define stack_index(s, i) s.stack[i]
#define modtop(s, v) s.stack[s.sp - 1] = v

/* Mem pools */
//#define slice(t, n) struct n {t *d; int i; int cap;}
//slice(int, int_slice);

#define pooltype(t, name) struct name {t *pool;\
	int cap;\
	int p_index;\
	stacktype(int) free_blocks;\
}

/* #define pool_new(t, name, sz) t name;\
	name.pool              = new(t, sz);\
	name.free_blocks.stack = new(t, sz)

	*/

#define pool(p) memset(&(p)->pool[(p)->p_index++], 0, sizeof *(p)->pool)
#define pool2(p, x) {\
	if ((p)->free_blocks.sp) {\
		*x = memset(&(p)->pool[pop((p)->free_blocks)], 0, sizeof *(p)->pool);\
	} else {\
		*x = memset(&(p)->pool[(p)->p_index++], 0, sizeof *(p)->pool);\
	}\
}
#define pool_free(p, i) do {\
	if (i < 0) break;\
	push((p)->free_blocks, i), memset(&(p)->pool[i], 0, sizeof(*(p)->pool));\
	} while(0)\

#define pool_stay(p)      &p->pool[p->p_index]
#define pool_index(p, i)  &p->pool[i]
#define pool_offset(p, o) &p->pool[p->p_index + o]
#define pool_top(p)       &p->pool[p->p_index - 1]
#define pool_inc(p)       p->p_index++
#define pool_freetop(p)   p->p_index--

#define pool_ptr_index(p, ptr, val) \
	*val = -1;\
	for (int it = 0; it < (p)->p_index; it++) {\
		if (&(p)->pool[it] == ptr) *val = it;\
	}

/* Strings */
#define streq(a,b) (strcmp((a),(b)) == 0)
#define strstarts(str,prefix) (strncmp((str),(prefix),strlen(prefix)) == 0)

#define size_of(x) (size) sizeof(x)
#define countof(x) size_of(x) / size_of(x[0])
#define for_range(lower, upper) for (int it = lower; it < upper; it++)

/* Log */
#define errlog(...) fprintf(stderr, __VA_ARGS__)

static inline bool strends(const char *str, const char *postfix)
{
	if (strlen(str) < strlen(postfix))
		return false;

	return streq(str + strlen(str) - strlen(postfix), postfix);
}

#endif
