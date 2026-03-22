#include "../lykosapi.h"
#include "../../src/vendor/font.h"

bool streq(char *s, char *s2) {
	while (*s++ && *s2++) {
		if (*s != *s2) return false;
	}
	return true;
}

typedef struct vector2 {
	int x;
        int y;
} vector2;

typedef struct rectangle {
	int x;
	int y; 
	int w;
	int h;
} rectangle;

enum node_flags {
	W_visible = 0x1,
	W_draw_decoration = 0x2,
	N_title = 0x4,
	//W_should_steal_focus = 0x4
};

typedef struct node node;
struct node {
	union {
		char *title;
		int id;
	};
	rectangle rec;
	union {
		//Window
		//bool *user_bool;
		//Text input
		struct {
			char *buffer;
			int  buff_i;
		};

		uint32_t texture[500 * 300];
	};
	//node *parent;
	unsigned int flags;
};

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


#define width 1920
#define height 1080
uint32_t *pixels;
int draw_width = width;

typedef struct color {
	uint8_t r, g, b;
} color;
color WHITE = {255, 255, 255};
color deccolour = {132, 133, 119};

void draw_pixel(int x, int y, color c) {
	if (x <= 0 || y <= 0 || x >= width || y >= height) return;
	pixels[x + (y * draw_width)] = *(uint32_t *) &c;
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

void draw_line_vec(vector2 start, vector2 end, color c) {
	draw_line(start.x, end.x, start.y, end.y, c);
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

void draw_decoration(node *n) { 
	draw_rect_lines(n->rec, deccolour);
	rectangle window_rec = n->rec;
	rectangle decoration = {0, 0, 0, 0};
	decoration = (rectangle){window_rec.x, window_rec.y - 20, window_rec.w, 20};
	draw_rectangle(decoration, deccolour);
	draw_rect_lines(decoration, deccolour);

	//rectangle close_button = {window_rec.x + window_rec.w - 25, window_rec.y - 19, 20, 19};
	//draw_rectangle(close_button, (color){0, 0, 255});
	//draw_rect_lines(close_button, WHITE);
	//draw_line_vec((vector2){close_button.x + close_button.w / 4, close_button.y + close_button.h / 4}, (vector2){close_button.x + close_button.w * 0.75, close_button.y + close_button.h - close_button.h / 4}, WHITE);
	//draw_line_vec((vector2){close_button.x + close_button.w / 4, close_button.y + close_button.h - close_button.h / 4}, (vector2){close_button.x + close_button.w * 0.75, close_button.y + close_button.h / 4}, WHITE);

	draw_string(n->title, n->rec.x + 5, n->rec.y - 15, WHITE);
}

void draw_background(node *n) {
	color c = {11, 11, 11};
	draw_rectangle((rectangle){0, 0, n->rec.w, n->rec.h}, c);
}

void draw_window(node *n) {
	draw_decoration(n);
}

bool in_rectangle(int x, int y, rectangle r) {
	if (x >= r.x && x <= r.x + r.w && y >= r.y && y <= r.y + r.h) return true;
	else return false;
}

u32 buf[width * height] = {0};

node nodes[3];
int node_c = 0;

bool node_exists(char *title, node **n) {
	int i;
	for (i = 0; i < node_c; i++) {
		if (nodes[i].title && streq(nodes[i].title, title)) {
			*n = &nodes[i];
			return true;
		}
	}
	return false;
}

node *window(char *title, int x, int y, int w, int h, unsigned int flags) {
	node *np;
	if (!node_exists(title, &np)) {
		np = &nodes[node_c++];
		np->title = title;
		np->rec = (rectangle){x, y, w, h};
		np->flags = flags;
	}
	return np;
}

void draw_texture(node *n) {
	int x, y, x1, y1;
	x1 = 0;
	y1 = 0;
	x = n->rec.x;
	y = n->rec.y;
	int i;
	for (i = 0; y1 < n->rec.h; i++) {
		pixels[x + (y * width)] = n->texture[x1 + (y1 * n->rec.w)];
		if (x1 == n->rec.w - 1) {
			y++;
			y1++;
			x = n->rec.x;
			x1 = 0;
		} else {
			x++;
			x1++;
		}
	}
}

void main(int argc, char **argv) {
	u32 *fb = mmap_fb();
	pixels = buf;
	rectangle r = {100, 100, 500, 300};
	rectangle deco = {r.x, r.y - 20, r.w, 20};
	color c = {0, 100, 100};
	for (int i = 0; i < width*height; i++) {
		fb[i] = BG;
	}

//	node *window = &nodes[node_i++];
//	window->rec = r;
//	window->title = "mterm";
//	window->flags |= W_visible;
//	static u32 term_buf[500 * 300];
//	window.buf = term_buf;
	for (;;) {
		for (int i = 0; i < width*height; i++) {
			buf[i] = BG;
		}
		node *win = window("mterm", 100, 100, 500, 300, W_visible | N_title);
		pixels = win->texture;
		draw_width = win->rec.w;
		draw_background(win);
		pixels = buf;
		draw_width = width;

		node *win2 = window("term", 100, 100, 500, 300, W_visible | N_title);
		pixels = win2->texture;
		draw_width = win2->rec.w;
		draw_background(win2);
		pixels = buf;
		draw_width = width;
		win->rec.x += 1;
		win->rec.y += 1;

		int i;
		for (i = 0; i < node_c; i++) {
			node *np = &nodes[i];
			if (np->flags & W_visible) {
				draw_texture(np);
				draw_decoration(np);
			}
		}
		//draw_string("StarShell >", win->rec.x + 5, win->rec.y + 5, WHITE);

		//if (win->rec.x == 200) win->flags &= ~W_visible;
		memcpy2((void *) fb, (void *) buf, sizeof buf);
		sleep(16);
	}
	return;
}
