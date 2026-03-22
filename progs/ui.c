#include "lykosapi.h"
#include "../src/vendor/font.h"

void memcpy2(char *src, char *buf, size_t sz) {
	int i;
	for (i = 0; i < sz; i++) {
		src[i] = buf[i];
	}
	return;
}

#define RED 0xFF0000
#define GREEN 0x00FF00
#define GREY 0x47544a
#define BLACK 0x0
#define BG 0x482459

typedef struct rectangle {
	int x;
	int y; 
	int w;
	int h;
} rectangle;

#define width 1920
#define height 1080
uint32_t *pixels;

typedef struct color {
	uint8_t r, g, b;
} color;
color WHITE = {255, 255, 255};

void draw_pixel(int x, int y, color c) {
	if (x <= 0 || y <= 0 || x >= width || y >= height) return;
	pixels[x + (y * width)] = *(uint32_t *) &c;
	return;
}

void draw_line(int start_x, int end_x, int start_y, int end_y, color c) {
	int x, y;
	x = start_x;
	y = start_y;
	while (x != end_x || y != end_y) {
		draw_pixel(x, y, c);
		if (x < end_x) x++;
		if (y < end_y) y++;
		if (y > end_y) y--;
		if (x > end_x) x--;
	}
	draw_pixel(x, y, c);
	return;
}

void draw_rectangle(rectangle r, color c) {
	int i;
	for (i = 0; i < r.h; i++) {
		draw_line(r.x, r.x + r.w, r.y + i, r.y + i, c);
	}
	return;
}

void draw_rect_lines(rectangle r, color c) {
	draw_line(r.x, r.x + r.w, r.y, r.y, c);
	draw_line(r.x, r.x, r.y, r.y + r.h, c);
	draw_line(r.x, r.x + r.w, r.y + r.h, r.y + r.h, c);
	draw_line(r.x + r.w, r.x + r.w, r.y + r.h, r.y, c);
}

void draw_decoration(rectangle window_rec, color c) {
	color deccolour;
	deccolour = (color){119, 133, 132, 255};
	rectangle decoration = {0, 0, 0, 0};
	decoration = (rectangle){window_rec.x, window_rec.y - 20, window_rec.w, 20};
	draw_rectangle(decoration, c);
	draw_rect_lines(decoration, c);

}

void draw_char_scaled(char c, size_t px, size_t py, color co, size_t scale) {
	if (c < 0)
		return;

	for (size_t row = 0; row < 8; row++) {
		u8 bits = font8x8_basic[(size_t)c][row];
		for (size_t col = 0; col < 8; col++) {
			if (bits & (1 << col)) {
				// Draw a scale x scale block instead of a single pixel
				for (size_t dy = 0; dy < scale; dy++) {
					for (size_t dx = 0; dx < scale; dx++) {
						draw_pixel(px + col * scale + dx, py + row * scale + dy, co);
					}
				}
			}
		}
	}
}

void draw_string(char *str, size_t px, size_t py, color c) {
	int g_scale = 1;
	while (*str) {
		if (px > (width - (7 * g_scale))) {
			px = 0;
			py += 8 * g_scale;
		}
		if (*str == '\n') {
			py += 8 * g_scale;
			px = 0;
		} else {
			draw_char_scaled(*str, px, py, c, g_scale);
			px += 8 * g_scale;
		}
		str++;
	}
}


bool in_rectangle(int x, int y, rectangle r) {
	if (x >= r.x && x <= r.x + r.w && y >= r.y && y <= r.y + r.h) return true;
	else return false;
}

u32 buf[width * height] = {0};

void main(int argc, char **argv) {
	u32 *fb = mmap_fb();
	pixels = buf;
	rectangle r = {100, 100, 500, 300};
	rectangle deco = {r.x, r.y - 20, r.w, 20};
	color c = {0, 100, 100};
	for (int i = 0; i < width*height; i++) {
		fb[i] = BG;
	}
	for (;;) {
		for (int i = 0; i < width*height; i++) {
			buf[i] = BG;
		}
		r.x += 5;
		r.y += 5;

		draw_rectangle(r, c);
		draw_rect_lines(r, WHITE);
		draw_decoration(r, WHITE);
		//draw_char_scaled('f', 110, 110, WHITE, 1);
		draw_string("StarShell >", r.x + 5, r.y + 5, WHITE);
		draw_string("mterm", r.x + 5, r.y - 15, (color){0, 0, 0});
		memcpy2((void *) fb, (void *) buf, sizeof buf);
	}
	return;
}
