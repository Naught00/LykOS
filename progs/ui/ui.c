#include "../lykosapi.h"
#include "../../src/vendor/font.h"
#include <ctype.h>
#include "keys.h"
#include "mn.h"

u8 img_buffer[] = {
#embed "snow.jpg"
};
u8 scaled_image[] = {
#embed "scaled_image.jpg"
};


size_t strlen(const char *s) {
	int i;
	while (*s++) i++;
	return i;
}

//bool streq(char *s1, char *s2) {
//	if (!s1 || !s2) return false;
//	while (*s1++ == *s2++)  {
//		if (!*s1 && !*s2) return true;
//	}
//	return false;
//}

//void *memcpy(void *restrict dest, const void *restrict src, size_t n) {
//	u8 *restrict pdest = (u8 *restrict)dest;
//	const u8 *restrict psrc = (const u8 *restrict)src;
//
//	for (size_t i = 0; i < n; i++) {
//		pdest[i] = psrc[i];
//	}
//
//	return dest;
//}
//void memcpy2(char *src, char *buf, size_t sz) {
//	int i;
//	for (i = 0; i < sz; i++) {
//		src[i] = buf[i];
//	}
//	return;
//}
void *memset(void *s, int c, size_t n) {
	u8 *p = (u8 *)s;

	for (size_t i = 0; i < n; i++) {
		p[i] = (u8)c;
	}

	return s;
}

void _assert(bool b) {
	if (!b) lykos_exit();
}

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

#define RED 0xFF0000
#define GREEN 0x00FF00
#define GREY 0x47544a
#define BG 0x2a45bf


#define width 1920
#define height 1080
uint32_t *pixels;
int draw_width = width;

typedef struct color {
	uint8_t r, g, b;
} color;
color WHITE = {255, 255, 255};
color BLACK = {0, 0, 0};
color dark_background = {11, 11, 11};

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
	N_text = 0x8,
	N_focused = 0x10,
	W_draw_border = 0x20,
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
			char buffer[256];
			int  buff_i;
		};
		uint32_t texture[1920 * 1080];

	};
	node *parent;
	unsigned int flags;
};


node nodes[10];
int node_c = 0;
node *windows[10];
int win_c = 0;
int focused_window;

void draw_pixel(int x, int y, color c) {
	if (x < 0 || y < 0 || x >= width || y >= height) return;
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
		//if (px > (width - (7 * g_scale))) {
		//	px = 0;
		//	py += 8 * g_scale;
		//}
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

void draw_border(node *n) {
	color deccolour;
	if (n == windows[focused_window]) {
		deccolour = (color){132, 133, 119};
	} else {
		deccolour = (color){34, 34, 34};
	}
	draw_rect_lines(n->rec, deccolour);
}

void draw_decoration(node *n) { 
	color deccolour;
	if (n == windows[focused_window]) {
		deccolour = (color){132, 133, 119};
	} else {
		deccolour = (color){34, 34, 34};
	}
	//draw_rect_lines(n->rec, deccolour);
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

void draw_background(node *n, color c) {
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


bool node_exists(char *title, node **n) {
	int i;
	for (i = 0; i < node_c; i++) {
		if (nodes[i].flags & N_title && streq(nodes[i].title, title)) {
			*n = &nodes[i];
			return true;
		}
	}
	return false;
}

bool node_exists_id(int id, node **n) {
	int i;
	for (i = 0; i < node_c; i++) {
		if (nodes[i].flags & N_text && nodes[i].id == id) {
			*n = &nodes[i];
			return true;
		}
	}
	return false;
}

unsigned int defwinflags = W_visible | N_title | W_draw_decoration | W_draw_border;

node *window(char *title, int x, int y, int w, int h, unsigned int flags) {
	node *np;
	if (!node_exists(title, &np)) {
		np = &nodes[node_c++];
		np->title = title;
		np->rec = (rectangle){x, y, w, h};
		np->flags = flags;
		np->parent = np;
		windows[win_c++] = np;
	}
	return np;
}

node *text_input(int id, node *parent, char *base, rectangle rec, unsigned int flags) {
	node *np;
	if (!node_exists_id(id, &np)) {
		np = &nodes[node_c++];
		np->id = id;
		np->rec = rec;
		np->flags = N_text | flags;
		np->buff_i = 0;
		np->parent = parent;
	}
	return np;
}
void draw_texture_pix(u32 *texture, int ox, int oy, int w, int h) {
	int x, y, x1, y1;
	x1 = 0;
	y1 = 0;
	x = ox;
	y = oy;
	int i;
	bool out_bounds;
	for (i = 0; y1 < h; i++) {
		out_bounds = x < 0 || y < 0 || x >= width || y >= height;
		if (!out_bounds) 
			pixels[x + (y * draw_width)] = texture[x1 + (y1 * w)];
		if (x1 == w - 1) {
			y++;
			y1++;
			x = ox;
			x1 = 0;
		} else {
			x++;
			x1++;
		}
	}
}

void draw_texture(node *n) {
	draw_texture_pix(n->texture, n->rec.x, n->rec.y, n->rec.w, n->rec.h);
	//int x, y, x1, y1;
	//x1 = 0;
	//y1 = 0;
	//x = n->rec.x;
	//y = n->rec.y;
	//int i;
	////int tex_len = n->rec.w * n->rec.h;
	//bool out_bounds = false;
	//for (i = 0; y1 < n->rec.h; i++) {
	//	//fixme draw_width/height
	//	out_bounds = x < 0 || y < 0 || x >= width || y >= height;
	//	if (!out_bounds) 
	//		pixels[x + (y * draw_width)] = n->texture[x1 + (y1 * n->rec.w)];

	//	if (x1 == n->rec.w - 1) {
	//		y++;
	//		y1++;
	//		x = n->rec.x;
	//		x1 = 0;
	//	} else {
	//		x++;
	//		x1++;
	//	}
	//}
}


void set_node_target(node *n) {
	pixels = n->texture;
	draw_width = n->rec.w;
}

void set_pix_target(uint32_t *p) {
	pixels = p;
	draw_width = width;
}

void set_focus(node *n) {
	int i;
	for (i = 0; i < win_c; i++) {
		if (n == windows[i]) {
			focused_window = i;
		}
	}
}

void bgr_to_rgb(byte *bgr, int w, int h) {
	int i;
	size len = w * h * 4;
	for (i = 0; i < len; i += 4) {
		byte tmp = bgr[i];
		bgr[i] = bgr[i + 2];
		bgr[i + 2] = tmp;
	}
}


void main(int argc, char **argv) {
	u32 *fb = mmap_fb();
	int iw, ih, ic;

	u8 *scaled = stbi_load_from_memory(scaled_image, sizeof scaled_image, &iw, &ih, &ic, 4);
	bgr_to_rgb(scaled, iw, ih);
	node *imgviewer = window("Image Viewer", 200, 300, iw, ih, defwinflags);
	set_pix_target(imgviewer->texture);
	draw_width = iw;
	draw_texture_pix((u32 *)scaled, 0, 0, iw, ih);
	draw_width = 1920;

	u8 *image = stbi_load_from_memory(img_buffer, sizeof img_buffer, &iw, &ih, &ic, 4);
	bgr_to_rgb(image, iw, ih);
	node *surface = window("surface", 0, 0, iw, ih, W_visible | N_title);
	set_pix_target(surface->texture);
	draw_width = iw;
	draw_texture_pix((u32 *)image, 0, 0, iw, ih);
	draw_width = 1920;


	pixels = buf;
	rectangle r = {100, 100, 500, 300};
	rectangle deco = {r.x, r.y - 20, r.w, 20};
	color c = {0, 100, 100};
	for (int i = 0; i < width*height; i++) {
		fb[i] = BG;
	}
	//u64 *mem = mmap(256);

//	node *window = &nodes[node_i++];
//	window->rec = r;
//	window->title = "mterm";
//	window->flags |= W_visible;
//	static u32 term_buf[500 * 300];
//	window.buf = term_buf;
	for (;;) {
		node *launcher = window("launcher", width / 2 - 250, height / 2 - 10, 500, 20, W_visible | N_title | W_draw_border);
		//for (int i = 0; i < width*height; i++) {
		//	buf[i] = BG;
		//}
		vector2 diff = {0};
		KeyEvent ev;
		char evbuf[16];
		int evbufi = 0;
		while (1) {
			i64 ret = get_key_event(&ev);
			if (ret == 0) {
				break;
			} else if (ev.key == KEY_UP_ARROW) {
				diff.y -= 10;
			} else if (ev.key == '\t') {
				if (focused_window < win_c - 1)
					focused_window++;
				else focused_window = 0;
			} else if (ev.key == 27) {
				lykos_exit();
			} else if (ev.key == 'q') {
				if (windows[focused_window]) {
					node *win = windows[focused_window];
					if (win->flags & W_visible) {
						win->flags &= ~W_visible;
					} else {
						win->flags |= W_visible;
					}
				}
			} else if (ev.key == 'd' && ev.modifiers & MOD_CTRL) {
				if (launcher->flags & W_visible) {
					launcher->flags &= ~W_visible;
				} else {
					launcher->flags |= W_visible;
					set_focus(launcher);
					
				}
			} else if (ev.key == KEY_DOWN_ARROW) {
				diff.y += 10;
			} else if (ev.key == KEY_LEFT_ARROW) {
				diff.x -= 10;
			} else if (ev.key == KEY_RIGHT_ARROW) {
				diff.x += 10;
			} else {
				evbuf[evbufi++] = ev.key;
			}
		}

		if (launcher->flags & W_visible) {
			set_node_target(launcher);
			draw_background(launcher, WHITE);
			node *n;
			n = text_input(3, launcher, "", launcher->rec, N_focused);
			int x = 0;
			int j = 0;
			char c;
			while (c = n->buffer[j++]) {
				if (c == '\n') {
					memset(n->buffer, 0, 256);
					n->buff_i = 0;
				}
				draw_char_scaled(c, x, 5, BLACK, 1);
				x += 8;
			}
			set_pix_target(buf);
		}
		node *win = window("mterm", 100, 100, 500, 300, defwinflags);
		if (win->flags & W_visible)
		{
			set_node_target(win);
			draw_background(win, dark_background);
			char *prompt = "/user> ";
			int prompt_len = strlen(prompt) * 8;
			draw_string(prompt, 0, 0, WHITE);
			char *a;
			node *n;
			n = text_input(1, win, "", win->rec, N_focused);
			a = n->buffer;

			int x = prompt_len;
			int y = 0;
			while (*a) {
				if (*a == '\n') {
					y += 8;
					x = 0;
					draw_string(prompt, x, y, WHITE);
					x += prompt_len;
				} else {
					draw_char_scaled(*a, x, y, WHITE, 1);
					x += 8;
				}
				*a++;
			}
			set_pix_target(buf);
		}

		node *win2 = window("xd", 400, 600, 500, 300, defwinflags);
		if (win2->flags & W_visible)
		{
			set_node_target(win2);
			draw_background(win2, dark_background);
			//draw_texture(surface);
			char *a;
			node *n;
			n = text_input(2, win2, "", win2->rec, 0);
			a = n->buffer;

			int x= 0;
			int y= 0;
			while (*a++) {
				if (*a == '\n') {
					y += 8;
					x = 0;
				} else {
					draw_char_scaled(*a, x, y, WHITE, 1);
					x+=8;
				}
			}
			set_pix_target(buf);
		}
		windows[focused_window]->rec.x += diff.x;
		windows[focused_window]->rec.y += diff.y;
		imgviewer->rec.x += 3;
		imgviewer->rec.y += 3;
		
		node *bar = window("bar", 0, 0, width, 20, W_visible | N_title);
		set_node_target(bar);
		draw_background(bar, WHITE);
		draw_string("Applications File Edit View", 5, 5, BLACK);
		draw_string(windows[focused_window]->title, 30 * 8 , 5, BLACK);
		set_pix_target(buf);

		int i;
		for (i = 0; i < node_c; i++) {
			node *np = &nodes[i];
			bool has_focus = np->parent == windows[focused_window];
			if (has_focus && np->flags & N_text && evbufi) {
				for (int x = 0; x < evbufi; x++) {
					np->buffer[np->buff_i++] = evbuf[x];
				}
			}

			if (np->flags & W_visible && np->flags & N_title && !has_focus) {
				draw_texture(np);
				if (np->flags & W_draw_decoration)
					draw_decoration(np);
				if (np->flags & W_draw_border)
					draw_border(np);
			}
		}
		for (i = 0; i < node_c; i++) {
			node *np = &nodes[i];
			bool has_focus = np->parent == windows[focused_window];
			if (np->flags & W_visible && np->flags & N_title && has_focus) {
				draw_texture(np);
				if (np->flags & W_draw_decoration)
					draw_decoration(np);
				if (np->flags & W_draw_border)
					draw_border(np);
			}
		}

		memcpy((void *) fb, (void *) buf, sizeof buf);
	}
	return;
}
