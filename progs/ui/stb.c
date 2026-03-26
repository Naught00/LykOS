#include "syscalls.h"

void _assert(bool b);

#include "kalloc.h"

#define STBI_NO_STDIO
#define STBI_NO_THREAD_LOCALS
#define STB_IMAGE_IMPLEMENTATION
#define STB_ASSERT(x) _assert(x)
#define STBI_MALLOC kalloc
#define STBI_REALLOC krealloc
#define STBI_FREE kfree
#include "stb_image.h"

#define STB_TRUETYPE_IMPLEMENTATION
#define STBTT_assert(x) _assert(x)
#define STBTT_malloc(x, u) kalloc(x)
//#define STBTT_realloc krealloc
#define STBTT_free(x, u) kfree(x)
#include "stb_truetype.h"
