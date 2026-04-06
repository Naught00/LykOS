void _assert(bool b);
#include "../../userspace/libraries/libc/lib/malloc.c"

#define STBI_NO_STDIO
#define STBI_NO_THREAD_LOCALS
#define STB_IMAGE_IMPLEMENTATION
#define STB_ASSERT(x) _assert(x)
#define STBI_MALLOC malloc
#define STBI_REALLOC realloc
#define STBI_FREE free
#include "stb_image.h"

#define STB_TRUETYPE_IMPLEMENTATION
#define STBTT_assert(x) _assert(x)
#define STBTT_malloc(x, u) malloc(x)
//#define STBTT_realloc krealloc
#define STBTT_free(x, u) free(x)
#include "stb_truetype.h"
