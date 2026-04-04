#ifndef  TEXT_C
#define  TEXT_C
#include "mwm/client.c"
#include "shapes.h"
#include "basic.h"
#include "lykosapi.h"
#include "libc/lib/malloc.c"
#include "libc/lib/assert.c"
#define STB_TRUETYPE_IMPLEMENTATION
#define STBTT_assert(x) _assert(x)
#define STBTT_malloc(x, u) kalloc(x)
//#define STBTT_realloc krealloc
#define STBTT_free(x, u) kfree(x)
#include "vendor/stb_truetype.h"

#include "graphics.c"

#define FONT_SIZE 18
#define BITMAP_SIZE 512
#define size ssize


void blit_surface(rectangle *dest, u8 *input, rectangle *src, bool black) {
	color c;
	int x, y;
	for (y = 0; y < dest->h; y++) {
		for (x = 0; x < dest->w; x++) {
			int source_x = src->x + x;
			int source_y = src->y + y;
			byte b = input[source_x + (source_y * BITMAP_SIZE)];
			if (black) {
				b = 255 - b;
			}
			c.r = b; c.g = b; c.b = b; c.a = 255;
			if (black && b < 255) draw_pixel(dest->x + x, dest->y + y, c);
			if (!black && b) draw_pixel(dest->x + x, dest->y + y, c);
		}
	}
	return;
}

typedef struct Font {
	stbtt_bakedchar cdata[96]; // ASCII 32..126 is 95 glyphs
	u8 *bitmap;
	int font_size;
	int bitmap_size;
	bool init;
} Font;

//@Todo request system font from wm
u8 _regular_font_file[] = {
#embed "../fonts/regular.ttf"
};
u8 _mono_font_file[] = {
#embed "../fonts/mono.ttf"
};
Font _regular;
Font _mono;

Font *_selected_font = &_regular;

enum font_type {
	MONO,
	REGULAR,
	ITALIC,
	BOLD,
};


u8 *load_font_mem(stbtt_bakedchar *cdata, u8 *font) {
	u8 *bitmap = mmap2(BITMAP_SIZE * BITMAP_SIZE);
	stbtt_BakeFontBitmap(font, 0, FONT_SIZE, bitmap, BITMAP_SIZE, BITMAP_SIZE, 32, 96, cdata);
	return bitmap;
}

void _init_font(Font *f, u8 *font_file) {
	if (f->init) return;

	f->bitmap = load_font_mem(f->cdata, font_file);
	f->bitmap_size = BITMAP_SIZE;
	f->font_size   = FONT_SIZE;
	f->init = true;
	return;
}

void set_font(enum font_type type) {
	switch (type) {
	case MONO:
		_selected_font = &_mono;
		_init_font(&_mono, _mono_font_file);
		break;
	case REGULAR:
		_selected_font = &_regular;
		_init_font(&_regular, _regular_font_file);
		break;
	}
}

void draw_text_ex(Font *font, char *text, bool black, float *x, float *y) {
	while (*text) {
		if (*text == '\n') {
			*y += font->font_size;
			*x = 0;
		}
		if (*text == '\t') {
			*x += 4 * font->font_size;
		}
		if (*text >= 32 && *text < 128) {
			stbtt_aligned_quad q;
			stbtt_GetBakedQuad(font->cdata, font->bitmap_size, font->bitmap_size, *text-32, x,y,&q,1);//1=opengl & d3d10+,0=d3d9
			int w = q.x1-q.x0;
			int h = q.y1-q.y0;

			/* t0,s0 and t1,s1 are texture-space coordinates, that is floats from
			   0.0-1.0. We have to scale them back to the pixel space used in the
			   glyph data bitmap. We multiply by the glyph bitmap
			   dimensions */
			rectangle src  = {.x = q.s0*font->bitmap_size, .y = q.t0*font->bitmap_size, .w = w, .h = h };

			/* In gl/d3d the y value is inverted compared to what we expect. y0
			   is negative here. We add it to the baseline to get the
			   correct  position to blit to. */
			rectangle dest = {.x = q.x0, .y = font->font_size+q.y0, .w = w, .h = h };

			blit_surface(&dest, font->bitmap, &src, black);
		}

		++text;
	}
	return; 
}

void draw_text_pro(char *text, bool black, float *x, float *y) {
	if (_selected_font == &_regular) _init_font(&_regular, _regular_font_file);
	draw_text_ex(_selected_font, text, black, x, y);
}

void draw_text(char *text, bool black, float x, float y) {
	if (_selected_font == &_regular) _init_font(&_regular, _regular_font_file);
	draw_text_ex(_selected_font, text, black, &x, &y);
}


#define _termbuffer_index(index) TERMBUFFER[index % sizeof TERMBUFFER]

void _term_move_head(char *buf, ssize len, int *head) {
	int i;
	int count = 0;
	for (i = *head; count < len; count++, i++) {
		if (buf[i % 4096] == '\n') {
			*head = (i + 1) % 4096;
			return;
		}
	}
}


void printf(char *fmt, ...) {
	static char TERMBUFFER[4096];
	static int TERM_HEAD;
	static int term_count;
	static u8 *TERM_BITMAP; 
	static s64 TERM_INDEX;
	static window *win;
	static bool terminal_init;
	if (!terminal_init) {
		//TERM_BITMAP = load_font_mem(baked_term_font);
		win = open_window("Terminal", 100, 100, 400, 600, -1);
		terminal_init = true;
	}
	Font *last_font = _selected_font;
	set_font(MONO);

	set_render_target(win);
        draw_background(WHITE);

	int max_lines = draw_height / FONT_SIZE;
	static int lines; 

	int j;
	char line[256] = {0};
	int len = strlen(fmt);
	for (j = 0; j < len; j++) {
		TERMBUFFER[TERM_INDEX % sizeof TERMBUFFER] = fmt[j];
		TERM_INDEX++;
		if (term_count < sizeof TERMBUFFER) {
			term_count++;
		} 
		if (fmt[j] == '\n') {
			if (lines >= max_lines) _term_move_head(TERMBUFFER, term_count, &TERM_HEAD);
			else lines++;
		}
	}

	float x = 0, y = 0;
	int count = 0;
	int i;
	for (i = TERM_HEAD, j = 0; count < term_count; count++) {
		char c = _termbuffer_index(i);
		if (!c) break;
		i++;
		line[j++] = c;
		if (c == '\n') {
			draw_text_pro(line, true, &x, &y);
			j = 0;
			memset(line, 0, sizeof line);
		}
	}
	commit_win(win);
	_selected_font = last_font;
	return;
}

#endif
