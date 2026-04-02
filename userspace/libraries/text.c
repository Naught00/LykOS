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

stbtt_bakedchar cdata[96]; // ASCII 32..126 is 95 glyphs

u8 *load_font_mem(u8 *font) {
	u8 *bitmap = mmap2(BITMAP_SIZE*BITMAP_SIZE);
	stbtt_BakeFontBitmap(font, 0, FONT_SIZE, bitmap, BITMAP_SIZE, BITMAP_SIZE, 32, 96, cdata);
	return bitmap;
}

void draw_text(u8 *bitmap, char *text, bool black, float *x, float *y) {
	while (*text) {
		if (*text == '\n') {
			*y += FONT_SIZE;
			*x = 0;
		}
		if (*text == '\t') {
			*x += 4 * FONT_SIZE;
		}
		if (*text >= 32 && *text < 128) {
			stbtt_aligned_quad q;
			stbtt_GetBakedQuad(cdata, BITMAP_SIZE,BITMAP_SIZE, *text-32, x,y,&q,1);//1=opengl & d3d10+,0=d3d9
			int w = q.x1-q.x0;
			int h = q.y1-q.y0;

			/* t0,s0 and t1,s1 are texture-space coordinates, that is floats from
			   0.0-1.0. We have to scale them back to the pixel space used in the
			   glyph data bitmap. We multiply by the glyph bitmap
			   dimensions */
			rectangle src  = {.x = q.s0*BITMAP_SIZE, .y = q.t0*BITMAP_SIZE, .w = w, .h = h };

			/* In gl/d3d the y value is inverted compared to what we expect. y0
			   is negative here. We add it to the baseline to get the
			   correct  position to blit to. */
			rectangle dest = {.x = q.x0, .y = FONT_SIZE+q.y0, .w = w, .h = h };

			blit_surface(&dest, bitmap, &src, black);
		}

		++text;
	}
	return; 
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

u8 baked_term_font[] = {
#embed "../fonts/mono.ttf"
};

void printf(char *fmt, ...) {
	static char TERMBUFFER[4096];
	static int TERM_HEAD;
	static int term_count;
	static u8 *TERM_BITMAP; 
	static s64 TERM_INDEX;
	static window *win;
	static bool terminal_init;
	if (!terminal_init) {
		TERM_BITMAP = load_font_mem(baked_term_font);
		win = open_window("Terminal", 100, 100, 400, 600, -1);
		terminal_init = true;
	}
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
			draw_text(TERM_BITMAP, line, true, &x, &y);
			j = 0;
			memset(line, 0, sizeof line);
		}
	}
	commit_win(win);
}

#endif
