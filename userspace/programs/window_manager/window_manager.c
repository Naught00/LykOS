#include <math.h>
#include "../../src/vendor/font.h"
#include <ctype.h>

#include "keys.h"
#include "basic.h"
#include "mwm/protocol.h"
#include "lykosapi.h"
#include "graphics.c"
#define TEXT_NO_STDIO
#include "text.c"
#define size ssize

#include "vendor/stb_truetype.h"

#define RED 0xFF0000
#define GREEN 0x00FF00
#define GREY 0x47544a
#define BG 0x2a45bf


#define width 1920
#define height 1080
#define MAX_WINDOWS 20

color dark_background = {11, 11, 11, 255};

typedef struct node node;
struct node {
	rectangle rec;
	char title[MAX_TITLE];
	int window_id;
	uint32_t texture[1920 * 1080];
	int client_mbox;
	uint32_t *shared_buf;
	unsigned int flags;
};


//0 is reserved for the background
//todo: should this be a seperate draw stack?
stack(node, 20) windows;
stack(int , MAX_WINDOWS) change_focus_subscribers;
int win_id_inc     = 1;
int focused_window = -1;


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
		//deccolour = (color){132, 133, 119, 255};
		//deccolour = (color){190, 190, 190, 255};
		//deccolour = (color){223, 223, 223, 255};
		deccolour = (color){255, 255, 255, 255};
	} else {
		deccolour = (color){34, 34, 34, 255};
	}
	draw_rect_lines(n->rec, deccolour);
}

void draw_decoration(node *n) { 
	color deccolour;
	bool black_text = false;
	if (n == &windows.stack[focused_window]) {
		//deccolour = (color){132, 133, 119, 255};
		//deccolour = (color){190, 190, 190, 255};
		//deccolour = (color){223, 223, 223, 255};
		deccolour = (color){255, 255, 255, 255};
		black_text = true;
	} else {
		deccolour = (color){34, 34, 34, 255};
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

	draw_text(n->title, black_text, n->rec.x + 5, n->rec.y - 22);
}

void draw_node_background(node *n, color c) {
	draw_rectangle((rectangle){0, 0, n->rec.w, n->rec.h}, c);
}

void draw_window(node *n) {
	draw_decoration(n);
}

u32 buf[width * height] = {0};

unsigned int defwinflags = W_visible | W_draw_decoration | W_draw_border | W_focusable | W_movable;

void send_msg(node *win, enum wm_msg_type type) {
	if (win->client_mbox < 0)
		return;

	wm_msg msg;
	msg.type = type;
	msg.window_id = win->window_id;
	mbox_send(win->client_mbox, &msg, sizeof msg);
	return;
}

node *window(char *title, int x, int y, int w, int h, unsigned int flags) {
	node *np;
	if (flags & W_background) {
		//node *current_background = &windows.stack[0];
		//if (current_background->shared_buf) {
		//	send_msg(current_background, WM_close);
		//}
		np = &windows.stack[0];
		*np = (node){0};
	} else {
		np = &stack_next(windows);
	}
	np->client_mbox = -1;
	strncpy(np->title, title, MAX_TITLE);
	np->window_id = win_id_inc++;
	np->rec = (rectangle){x, y, w, h};
	np->flags = flags; 
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
	if (win_index > -1) {
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

	volatile u32 *adjusted = pixels + ox + (oy * draw_width);
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
	int y, i;
	int offset = 0;
	int row_len;

	row_len = w * BPP;
	i = 0;
	if (ox + w >= draw_width) offset = ((ox + w) - draw_width) * BPP;
	for (y = oy; y < oy + h; y++) {
		if (y >= draw_height) break;
		if (y >= 0) memcpy((u32 *) pixels + ox + (y * draw_width), texture + i, row_len - offset);
		i += w;
	}
	return;
}

void wm_draw_texture_pix(u32 *texture, int ox, int oy, int w, int h) {
	bool same_size = w == draw_width && h == draw_height;
	if (same_size) {
		memcpy((void *) pixels, texture, draw_width * draw_height * BPP);
	} else {
		//fast_draw_texture_pix(texture, ox, oy, w, h);
		//memcpy_draw_texture_pix(texture, ox, oy, w, h);
		manual_draw_texture_pix(texture, ox, oy, w, h);
	}
}

void draw_texture(node *n) {
	wm_draw_texture_pix(n->texture, n->rec.x, n->rec.y, n->rec.w, n->rec.h);
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

//#include "3d.c"

void send_key(node *win, KeyEvent key) {
	if (win->client_mbox < 0)
		return;

	wm_msg msg = {0};
	msg.type = WM_key;
	msg.key_event = key;
	msg.window_id = win->window_id;
	mbox_send(win->client_mbox, &msg, sizeof msg);
	return;
}

void send_wm_msg(node *win, wm_msg *msg) {
	if (win->client_mbox < 0)
		return;

	mbox_send(win->client_mbox, msg, sizeof *msg);
	return;
}

void set_focus_index(int window_index) {
	int i;
	node *win;
	focused_window = window_index;

	win = &windows.stack[focused_window];
	for (i = 0; i < change_focus_subscribers.sp; i++) {
		int subscriber_mailbox = change_focus_subscribers.stack[i];

		wm_msg msg = {0};
		msg.type  = WM_change_focus;
		strncpy(msg.title, win->title, sizeof msg.title);
		mbox_send(subscriber_mailbox, &msg, sizeof msg);
	}
	return;
}

void set_focus(node *win) {
	int index = get_window_index_by_id(win->window_id);
	set_focus_index(index);
	return;
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
	if (win->flags & W_focusable) set_focus(win);
	int shmid = shm_create((w * h * BPP), true);
	if (shmid < 0) {
		write("Could not create shared memory region!\n");
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
	mbox_send(msg->mailbox, &response, sizeof response);
	return;
}


u64 keymap[3];
u64 ascii_to_key(char c, int *key_map_index) {
	u64 i = c - '!';
	u64 bit = 1 << i % 64;
	*key_map_index = i / 64;
	return bit;
}

void set_key_released(int k) {
	int index = 0;
	u32 key_bit;
	key_bit = ascii_to_key(k, &index);
	keymap[index] &= ~key_bit;
	return;
}

void set_key_pressed(int k) {
	int index = 0;
	u32 key_bit;
	key_bit = ascii_to_key(k, &index);
	keymap[index] |= key_bit;
	return;
}

bool key_pressed(int k) {
	int index = 0;
	u64 key_bit;
	key_bit = ascii_to_key(k, &index);
	if (keymap[index] & key_bit) {
		return true;
	} else {
		return false;
	}
}

bool key_released(int k) {
	int index = 0;
	u64 key_bit;
	key_bit = ascii_to_key(k, &index);
	if (keymap[index] & key_bit) {
		return false;
	} else {
		return true;
	}
}

void main(void) {
	windows.sp = 1;
	int mboxid = 0;
	int err = mbox_create(mboxid); 
	if (err < 0) {
		write("Requested mailbox id is already in use\n");
		lykos_exit();
	}
	exec("bground.elf");

	u32 *fb = mmap_fb();

	MailboxMessage out;
	wm_msg msg;
	i64 kb_size;
	volatile KeyEvent *kb = (KeyEvent *)mmap_keyboard((u64 *) &kb_size);
	i64 idx     = 0;
	i64 last_id = -1;
	while (1) {
		if (kb[idx].event_id > last_id) {
			last_id = kb[idx].event_id;
			idx++;
			if (idx >= kb_size) idx = 0;
		} else break;
	}

	int i;
	stack(node *, MAX_WINDOWS) clients_that_need_redraw = {{}, 0};
	exec("bar.elf");
	set_pix_target(buf);
	for (;;) {
		bool should_redraw_screen = false;
		while (mbox_receive(mboxid, &out, 0)) {
			//if (valid_msg)
			msg = *(wm_msg *) out.data;
			node *client = get_window_by_id(msg.window_id);
			switch (msg.type) {
			case WM_open:
				handle_open(&msg);
				break;
			case WM_close:
				if (!client) break;
				remove_window(msg.window_id);
				break;
			case WM_subscribe:
				if (msg.flags & WM_change_focus) {
					push(change_focus_subscribers, msg.mailbox);
					if (focused_window > -1) {
						node *focused = &windows.stack[focused_window];
						wm_msg response = {0};
						response.type  = WM_change_focus;
						strncpy(response.title, focused->title, sizeof response.title);
						mbox_send(msg.mailbox, &response, sizeof response);
					}
				}
				break;
			case WM_commit:
				if (!client) break;

				bool client_already_redrawing_this_frame = false;
				for (i = 0; i < clients_that_need_redraw.sp; i++)
					if (clients_that_need_redraw.stack[i]->window_id == client->window_id)
							client_already_redrawing_this_frame = true;


				if (!client_already_redrawing_this_frame) {
						push(clients_that_need_redraw, client);
						should_redraw_screen = true;
				}
				break;
			default: break;
			}
		}

		while (clients_that_need_redraw.sp) {
			node *client = pop(clients_that_need_redraw);
			set_node_target(client);
			wm_draw_texture_pix(client->shared_buf, 0, 0, client->rec.w, client->rec.h);
			set_pix_target(buf);
		}

		KeyEvent ev;
		while (1) {
			if (kb[idx].event_id > last_id) {
				last_id = kb[idx].event_id;
				ev = kb[idx];
				idx++;
				if (idx >= kb_size) idx = 0;

			} else {
				break;
			}
			if (ev.modifiers & MOD_RELEASE) {
				set_key_released(ev.key);
				if (focused_window > -1) {
					node *win = &windows.stack[focused_window];
					send_key(win, ev);
				}
				continue;
			} 

			bool alt = ev.modifiers & MOD_CTRL;

			if (ev.key == '\t') {
				int i, search_index;
				bool found = false;
				if (focused_window > -1) {
					search_index = focused_window;
				} else {
					search_index = 0;
				}
				for (i = 0; i < windows.sp; i++) {
					search_index++;
					if (search_index > windows.sp - 1) search_index = 0;
					node *win = &windows.stack[search_index];
					if (!(win->flags & W_focusable) || !(win->flags & W_visible)) {
						continue;
					} else {
						found = true;
						send_msg(&windows.stack[focused_window], WM_unfocus);
						set_focus_index(search_index);
						send_msg(win, WM_focus);
						should_redraw_screen = true;
						break;
					}
				}
				if (!found) focused_window = -1;
			} else if (alt && ev.key == KEY_ESCAPE) {
				write("Sending close events to clients...\n");
				for (i = 0; i < windows.sp; i++) {
					node *win = &windows.stack[i];
					send_msg(win, WM_close);
				}
				lykos_exit();
			} else if (alt && ev.key == 'q') {
				if (focused_window >= 0) {
					node *win = &windows.stack[focused_window];
					send_msg(win, WM_close);
					remove_window(win->window_id);
					should_redraw_screen = true;
				}
			} else if (alt && ev.key == 'd') {
				exec("launcher.elf");
			} else {
				set_key_pressed(ev.key);
				if (focused_window >= 0) {
					node *win = &windows.stack[focused_window];
					send_key(win, ev);
				} 
			}

		}
		if (focused_window >= 0) {
			node *focusedwin = &windows.stack[focused_window];
			rectangle *win_rec = &(windows.stack[focused_window].rec);
			if (focusedwin->flags & W_movable) {
				if (key_pressed(KEY_RIGHT_ARROW)) win_rec->x += 5;
				if (key_pressed(KEY_LEFT_ARROW))  win_rec->x -= 5;
				if (key_pressed(KEY_UP_ARROW))    win_rec->y -= 5;
				if (key_pressed(KEY_DOWN_ARROW))  win_rec->y += 5;

				should_redraw_screen = true;
			}
		}

		if (!should_redraw_screen) { 
			sleep(1);
			continue;
		}



		//if (win2->flags & W_visible)
		//{
		//	//if (uptime > 1000) {
		//	//	set_node_target(win2);
		//	//	draw_node_background(win2, dark_background);
		//	//	int fps = frames / (uptime / 1000);
		//	//	char x[256];
		//	//	snprintf(x, sizeof x, "%u", fps);
		//	//	draw_string(x, 5, 5, WHITE);
		//	//	set_pix_target(buf);
		//	//}
		//}

		for (i = 0; i < windows.sp; i++) {
			node *np = &windows.stack[i];
			if (!(np->flags & W_visible)) continue;
			if (focused_window > -1) if (np == &windows.stack[focused_window]) continue;

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
