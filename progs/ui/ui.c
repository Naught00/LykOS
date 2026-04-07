#include <math.h>
#include "../../src/vendor/font.h"
#include <ctype.h>

#include "keys.h"
#include "basic.h"
#include "mwm/protocol.h"
#include "../../userspace/libraries/lykosapi.h"
#define size ssize

void _assert(bool b) {
	if (!b) lykos_exit();
}
	

u8 img_buffer[] = {
#embed "snow.jpg"
};

#include "stb_image.h"
#include "stb_truetype.h"

#define RED 0xFF0000
#define GREEN 0x00FF00
#define GREY 0x47544a
#define BG 0x2a45bf


#define width 1920
#define height 1080
#define BPP 4
uint32_t *pixels;
int draw_width = width;
int draw_height = width;

typedef struct color {
	uint8_t r, g, b, a;
} color;
color WHITE = {255, 255, 255};
color BLACK = {0, 0, 0};
color dark_background = {11, 11, 11};

typedef struct vector2 {
	int x;
        int y;
} vector2;

typedef struct vector2f {
	float x;
        float y;
} vector2f;

typedef struct vector3f {
	float x;
        float y;
        float z;
} vector3f;

typedef struct rectangle {
	int x;
	int y; 
	int w;
	int h;
} rectangle;

enum node_flags {
	N_title = 0x20,
	N_text = 0x40,
	N_focused = 0x80,
};

typedef struct node node;
struct node {
	rectangle rec;
	char title[MAX_TITLE];
	int window_id;
	uint32_t texture[1920 * 1080];
	int client_mbox;
	uint32_t *shared_buf;
	node *parent;
	unsigned int flags;
};


stack(node, 20) windows;
int win_id_inc     = 0;
int focused_window = -1;

void draw_pixel(int x, int y, color c) {
	if (x < 0 || y < 0 || x >= draw_width || y >= draw_height) return;
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
	if (n == &windows.stack[focused_window]) {
		deccolour = (color){132, 133, 119};
	} else {
		deccolour = (color){34, 34, 34};
	}
	draw_rect_lines(n->rec, deccolour);
}

void draw_decoration(node *n) { 
	color deccolour;
	if (n == &windows.stack[focused_window]) {
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


unsigned int defwinflags = W_visible | N_title | W_draw_decoration | W_draw_border | W_focusable | W_movable;

node *window(char *title, int x, int y, int w, int h, unsigned int flags) {
	node *np;
	np = &stack_next(windows);
	np->client_mbox = -1;
	strncpy(np->title, title, MAX_TITLE);
	np->window_id = win_id_inc++;
	np->rec = (rectangle){x, y, w, h};
	np->flags = flags | N_title;
	np->parent = np;
	return np;
}

int get_window_index_by_id(int id) {
	int i;
	for (i = 0; i < windows.sp; i++) {
		node *win = &windows.stack[i];
		if (win->window_id == id) {
			return i;
		}
	}
	return -1;
}

void set_focus(node *n) {
	focused_window = get_window_index_by_id(n->window_id);
	return;
}

node *get_window_by_id(int id) {
	int i;
	for (i = 0; i < windows.sp; i++) {
		node *win = &windows.stack[i];
		if (win->window_id == id) {
			return win;
		}
	}
	return null;
}

void remove_window(int win_id) {
	int win_index = get_window_index_by_id(win_id);
	if (win_index >= 0) {
		int j = win_index;
		for (; j < windows.sp - 1; j++) {
			windows.stack[j] = windows.stack[j + 1];
		}
		windows.stack[j] = (node){0};
		windows.sp--;
		focused_window = -1;
	}
	return;
}


#include <emmintrin.h>
void fast_draw_texture_pix(u32 *texture, int ox, int oy, int w, int h) {
	int x, y;
	int row_len = (w * BPP) / 16;
	int out_len = (draw_width * BPP) / 16;
	__m128i *a = (__m128i *) texture;

	u32 *adjusted = pixels + ox + (oy * draw_width);
	__m128i *cursor = (__m128i *) adjusted;
	__m128i z;
	for (y = 0; y < h; y++) {
		for (x = 0; x < row_len; x++) {
			z = _mm_loadu_si128(a);
			a++;
			_mm_storeu_si128(cursor + x, z);
		}
		cursor += out_len;
	}
	return;
}

void memcpy_draw_texture_pix(u32 *texture, int ox, int oy, int w, int h) {
	int x, y, i;
	bool out_bounds;
	int offset = 0;
	int row_len;

	row_len = w * BPP;
	i = 0;
	if (ox + w >= draw_width) offset = ((ox + w) - draw_width) * BPP;
	for (y = oy; y < oy + h; y++) {
		if (y >= draw_height) break;
		if (y >= 0) memcpy(pixels + ox + (y * draw_width), texture + i, row_len - offset);
		i += w;
	}
	return;
}

void manual_draw_texture_pix(u32 *texture, int ox, int oy, int w, int h) {
	int i;
	int x, y;
	i = 0;
	for (y = oy; y < oy + h; y++) {
		for (x = ox; x < ox + w; x++, i++) {
			if (x < 0 || y < 0 || x >= draw_width || y >= draw_height) continue;
			else pixels[x + (y * draw_width)] = texture[i];
		}
	}
	return;
}


void draw_texture_pix(u32 *texture, int ox, int oy, int w, int h) {
	bool same_size = w == draw_width && h == draw_height;
	if (same_size) {
		memcpy(pixels, texture, draw_width * draw_height * BPP);
	} else {
		//fast_draw_texture_pix(texture, ox, oy, w, h);
		//memcpy_draw_texture_pix(texture, ox, oy, w, h);
		manual_draw_texture_pix(texture, ox, oy, w, h);
	}
}

void draw_texture(node *n) {
	draw_texture_pix(n->texture, n->rec.x, n->rec.y, n->rec.w, n->rec.h);
}


void set_node_target(node *n) {
	pixels = n->texture;
	draw_width = n->rec.w;
	draw_height = n->rec.h;
}

void set_pix_target(uint32_t *p) {
	pixels = p;
	//fixme
	draw_width  = width;
	draw_height = height;
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

void g8bpp_to_32bpp(u32 *out, u8 *in, int w, int h) {
	int i, x;
	size len = w * h * 4;
	u8 *outb = (u8 *) out;
	for (i = 0, x = 0; i < len; i += 4) {
		byte b = in[x++];
		if (!b) continue;
		outb[i] = b;
		outb[i + 1] = b;
		outb[i + 2] = b;
		outb[i + 3] = b;
	}
}

#include "3d.c"

void wallpaper() {
	int iw, ih, ic;

	u8 *image = stbi_load_from_memory(img_buffer, sizeof img_buffer, &iw, &ih, &ic, 4);
	bgr_to_rgb(image, iw, ih);
	node *surface = window("surface", 0, 0, iw, ih, W_visible | N_title);
	set_node_target(surface);
	draw_texture_pix((u32 *)image, 0, 0, iw, ih);
	set_pix_target(buf);
}

void handle_open(wm_msg *msg) {
	int x, y, w, h;
	if (msg->x < 0) x = (width / 2)  - msg->w / 2;
	else x = msg->x;
	if (msg->y < 0) y = (height / 2) - msg->h / 2;
	else y = msg->y;
	if (msg->w < 0) w = width;
	else w = msg->w;
	if (msg->h < 0) h = height;
	else h = msg->h;

	node *win = window(msg->title, x, y, w, h, msg->flags);
	set_focus(win);
	int shmid = shm_create((w * h * BPP), true);
	if (shmid < 0) {
		lykos_exit();
	}

	u64 sz;
	uint32_t *buf = shm_map(shmid, &sz);
	win->shared_buf = buf;
	win->client_mbox = msg->mailbox;

	//respond
	wm_msg response;
	response.type = WM_ok;
	response.shm_id = shmid;
	response.window_id = win->window_id;
	response.given_x = x;
	response.given_y = y;
	response.given_w = w;
	response.given_h = h;
	int ret = mbox_send(msg->mailbox, &response, sizeof response);
	return;
}

void handle_msg(wm_msg *msg) {
	switch (msg->type) {
	case WM_open:
		break;
	}
}

void send_key(node *win, KeyEvent key) {
	if (win->client_mbox < 0)
		return;

	wm_msg msg;
	msg.type = WM_key;
	msg.key_event = key;
	msg.window_id = win->window_id;
	mbox_send(win->client_mbox, &msg, sizeof msg);
	return;
}

void send_msg(node *win, enum wm_msg_type type) {
	if (win->client_mbox < 0)
		return;

	wm_msg msg;
	msg.type = type;
	msg.window_id = win->window_id;
	mbox_send(win->client_mbox, &msg, sizeof msg);
	return;
}


void main(int argc, char **argv) {
	u32 *fb = mmap_fb();
	wallpaper();
	int iw, ih, ic;

	int mboxid = 0;
	int err = mbox_create(mboxid); 
	if (err < 0) {
		write("Requested mailbox id is already in use\n");
		lykos_exit();
	}

	set_pix_target(buf);

	rectangle r = {100, 100, 500, 300};
	rectangle deco = {r.x, r.y - 20, r.w, 20};
	color c = {0, 100, 100};
	
	MailboxMessage out;
	wm_msg msg;
	u64 kb_size;
	volatile KeyEvent *kb = (KeyEvent *)mmap_keyboard(&kb_size);
	s64 idx     = 1;
	s64 last_id = -1;
	while (1) {
		if (kb[idx].event_id > last_id) {
			last_id = kb[idx].event_id;
			idx++;
			if (idx >= kb_size) idx = 0;
		} else break;
	}

	int i, j;
	exec("bar.elf");
	for (;;) {
		set_pix_target(buf);
		bool should_redraw_screen = false;
		while (mbox_receive(mboxid, &out, 0)) {
			//if (valid_msg)
			node *client;
			msg = *(wm_msg *) out.data;
			switch (msg.type) {
			case WM_open:
				handle_open(&msg);
				break;
			case WM_close:
				remove_window(msg.window_id);
				break;
			case WM_commit:
				client = get_window_by_id(msg.window_id);
				if (!client) break;
				uint32_t *cbuf = client->shared_buf;

				set_node_target(client);
				draw_texture_pix(cbuf, 0, 0, client->rec.w, client->rec.h);
				set_pix_target(buf);
				should_redraw_screen = true;
				break;
			default: break;
			}
		}

		vector2 diff = {0};
		KeyEvent ev;
		char evbuf[64] = {0};
		int evbufi = 0;
		while (1) {
			if (kb[idx].event_id > last_id) {
				last_id = kb[idx].event_id;
				ev = kb[idx];
				idx++;
				if (idx >= kb_size) idx = 0;
				if (ev.modifiers & MOD_RELEASE && focused_window >= 0) {
					node *win = &windows.stack[focused_window];
					send_key(win, ev);
					continue;
				} 

				should_redraw_screen = true;
			} else {
				break;
			}
			bool alt = ev.modifiers & MOD_CTRL;

			if (ev.key == '\t') {
				int i;
				bool found = false;
				if (focused_window == -1) focused_window = 0;
				else send_msg(&windows.stack[focused_window], WM_unfocus);
				for (i = 0; i < windows.sp; i++) {
					focused_window++;
					if (focused_window > windows.sp - 1) focused_window = 0;
					node *win = &windows.stack[focused_window];
					if (!(win->flags & W_focusable) || !(win->flags & W_visible)) {
						continue;
					} else {
						found = true;
						break;
					}
				}
				if (!found) focused_window = -1;
				else send_msg(&windows.stack[focused_window], WM_focus);
			} else if (ev.key == KEY_ESCAPE) {
				lykos_exit();
			} else if (alt && ev.key == 'q') {
				if (focused_window >= 0) {
					node *win = &windows.stack[focused_window];
					win->flags &= ~W_visible;
					send_msg(win, WM_close);
					remove_window(win->window_id);
				}
			} else if (alt && ev.key == 'd') {
				exec("launcher.elf");
			} else if (alt && ev.key == 'b') {
				exec("bar.elf");
			}  else if (ev.key == KEY_UP_ARROW) {
				diff.y -= 10;
			} else if (ev.key == KEY_DOWN_ARROW) {
				diff.y += 10;
			} else if (ev.key == KEY_LEFT_ARROW) {
				diff.x -= 10;
			} else if (ev.key == KEY_RIGHT_ARROW) {
				diff.x += 10;
			} else {
				if (focused_window >= 0) {
					node *win = &windows.stack[focused_window];
					send_key(win, ev);
				} 
			}
		}

		if (!should_redraw_screen) { 
			sleep(1);
			continue;
		}

	//	if (win2->flags & W_visible)
	//	{
	//		//if (uptime >= 1000) {
	//		//	set_node_target(win2);
	//		//	draw_background(win2, dark_background);
	//		//	int fps = frames / (uptime / 1000);
	//		//	char x[256];
	//		//	snprintf(x, sizeof x, "%u", fps);
	//		//	draw_string(x, 5, 5, WHITE);
	//		//	set_pix_target(buf);
	//		//	start = uptime_ms();
	//		//	frames = 0;
	//		//}
	//	}

		if (focused_window >= 0) {
			node *focuswin = &windows.stack[focused_window];
			if (focuswin->flags & W_movable) {
				(&windows.stack[focused_window])->rec.x += diff.x;
				(&windows.stack[focused_window])->rec.y += diff.y;
			}

		}
		for (i = 0; i < windows.sp; i++) {
			node *np = &windows.stack[i];
			if (!(np->flags & W_visible))
				continue;

			draw_texture(np);
			if (np->flags & W_draw_decoration)
				draw_decoration(np);
			if (np->flags & W_draw_border)
				draw_border(np);
		}
		//FIXME drawstack
		if (focused_window >= 0) {
			node *np = &windows.stack[focused_window];
			if (np->flags & W_visible) {
				draw_texture(np);
				if (np->flags & W_draw_decoration)
					draw_decoration(np);
				if (np->flags & W_draw_border)
					draw_border(np);
			}
		}

		memcpy(fb, buf, sizeof buf);
	}
	return;
}
