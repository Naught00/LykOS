#include "syscalls.h"
#include <math.h>
#include "../../src/vendor/font.h"
#include <ctype.h>
#include "keys.h"
#include "mn.h"


#include "../../userspace/libraries/mwm/protocol.h"

u8 img_buffer[] = {
#embed "snow.jpg"
};
u8 scaled_image[] = {
#embed "scaled_image.jpg"
};

u8 font_file[] = {
#embed "mono.ttf"
};


size_t strlen(const char *s) {
	int i = 0;
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
	uint8_t r, g, b;
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
	int id;
	rectangle rec;
	union {
		//Text input
		struct {
			char buffer[256];
			int  buff_i;
		};
		//Window
		struct {
			char *title;
			int window_id;
			uint32_t texture[1920 * 1080];
			int client_mbox;
			uint32_t *shared_buf;
		}

	};
	//??
	node *parent;
	unsigned int flags;
};



node nodes[20];
int node_c = 0;
node *windows[20];
//debug
node *clientwins[20];
int clientc = 0;
int win_c = 0;
int win_id_inc = 0;
int focused_window = -1;

//fixme: make these two stacks instead
void remove_window(node *win) {
	int i, j;
	for (i = 0; i < win_c; i++) {
		node *n = windows[i]; 
		if (n == win) {
			j = i + 1;
			//@Drawstack
			if (j == win_c) {
				windows[i] = null;
				focused_window--;
			} else {
				for (j = i + 1; j < win_c; j++) {
					windows[j - 1] = windows[j];
				}
			}
			win_c--;
			return;

		}

	}
	return;
}

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

unsigned int defwinflags = W_visible | N_title | W_draw_decoration | W_draw_border | W_focusable | W_movable;

node *window(char *title, int x, int y, int w, int h, unsigned int flags) {
	node *np;
	np = &nodes[node_c++];
	np->id = node_c - 1;
	np->client_mbox = -1;
	np->title = title;
	np->window_id = win_id_inc++;
	np->rec = (rectangle){x, y, w, h};
	np->flags = flags | N_title;
	np->parent = np;
	windows[win_c++] = np;
	return np;
}

void set_focus(node *n) {
	int i;
	for (i = 0; i < win_c; i++) {
		if (n == windows[i]) {
			focused_window = i;
		}
	}
}

node *get_window_by_id(int id) {
	int i;
	for (i = 0; i < win_c; i++) {
		node *win = windows[i];
		if (win->window_id == id) {
			return win;
		}
	}
	return null;
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
	bool same_size = w == draw_width && h == draw_height;
	if (same_size) {
		memcpy(pixels, texture, draw_width * draw_height * sizeof(u32));
		return;
	}
	int x, y, x1, y1;
	x1 = 0;
	y1 = 0;
	x = ox;
	y = oy;
	int i;
	bool out_bounds;
	for (i = 0; y1 < h; i++) {
		//fixme draw_width/height
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
	draw_height = n->rec.h;
}

void set_pix_target(uint32_t *p) {
	pixels = p;
	//fixme
	draw_width = width;
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

typedef stbtt_fontinfo Font;

Font init_font(u8 *font_file) {
	Font f;
	if (!stbtt_InitFont(&f, font_file, 0))
	{
	}
	return f;
}

u8 *make_bitmap() {
	stbtt_bakedchar cdata[96]; // ASCII 32..126 is 95 glyphs
	u8 *bitmap = mmap2(512*512);
	stbtt_BakeFontBitmap(font_file, 0, 32.0, bitmap, 512, 512, 32, 96, cdata);
	char *text = "test";
	float x = 0;
	float y = 0;
	u8 *chara = mmap2(512*512);
	while (*text) {
		if (*text >= 32 && *text < 128) {
			stbtt_aligned_quad q;
			stbtt_GetBakedQuad(cdata, 512,512, *text-32, &x,&y,&q,1);//1=opengl & d3d10+,0=d3d9
		}

		++text;
	}
	return bitmap;
}
void itxt(char *text, int x, int ty, stbtt_fontinfo *font) {
	u8 *fontbitmap = mmap2(draw_width * draw_height);
	int bitmapw = draw_width;
	int line_height = 18;
	float fscale = stbtt_ScaleForPixelHeight(font, line_height);
	int ascent, descent, linegap;
	stbtt_GetFontVMetrics(font, &ascent, &descent, &linegap);

	int i, j;
	ascent = roundf(ascent * fscale);
	descent = roundf(descent * fscale);
	int txt_y = ascent;
	for (j = 0; j < ty; j++) {
		txt_y += ascent - descent + linegap;
	}
	for (i = 0; i < strlen(text); i++) {
		if (text[i] == '\n') {
			txt_y += ascent -  descent + linegap;
			x = 0;
			continue;
		}
		int ax;
		int lsb;
		stbtt_GetCodepointHMetrics(font, text[i], &ax, &lsb);

		int c_x1, c_y1, c_x2, c_y2;
		stbtt_GetCodepointBitmapBox(font, text[i], fscale, fscale, &c_x1, &c_y1, &c_x2, &c_y2);

		int y = txt_y + c_y1;

		int byteOffset = x + roundf(lsb * fscale) + (y * bitmapw);
		stbtt_MakeCodepointBitmap(font, (u8 *) fontbitmap + byteOffset, c_x2 - c_x1, c_y2 - c_y1, bitmapw, fscale, fscale, text[i]);

		x += roundf(ax * fscale);

		int kern;
		kern = stbtt_GetCodepointKernAdvance(font, text[i], text[i + 1]);
		x += roundf(kern * fscale);
	}
	g8bpp_to_32bpp(pixels, fontbitmap, draw_width, draw_height);
	return;
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
	//@Arena
	char *title = mmap2(MAX_TITLE);
	strcpy(title, msg->title);
	int x, y;
	if (msg->x < 0 || msg->y < 0) {
		x = (width / 2)  - msg->w / 2;
		y = (height / 2) - msg->h / 2;
	} else {
		x = msg->x;
		y = msg->y;
	}

	node *win = window(title, x, y, msg->w, msg->h, msg->flags);
	set_focus(win);
	int shmid = shm_create((width * height * BPP), true);
	if (shmid < 0) {
		lykos_exit();
	}

	u64 sz;
	uint32_t *buf = shm_map(shmid, &sz);
	win->shared_buf = buf;
	clientwins[clientc++] = win;
	win->client_mbox = msg->mailbox;

	//respond
	wm_msg response;
	response.type = WM_ok;
	response.shm_id = shmid;
	response.window_id = win->window_id;
	int ret = mbox_send(msg->mailbox, &response, sizeof response);
	if (ret < 0)  {
		lykos_exit();
	}
	return;
}

void handle_msg(wm_msg *msg) {
	switch (msg->type) {
	case WM_open:
		break;
	}
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

//	int region = shm_create(640 * 480 * 4, true);
//	if (region < 0) {
//		lykos_exit();
//	}
//
//	u64 sz;
//	u32 *p = shm_map(region, &sz);
//	if (!p) lykos_exit();
//
////	char *hi = "hello from server\n";
////	strcpy(p, hi);
	//exec("client.elf");
	//exec("wexample.elf");
//	node *client = window("Client", 400, 700, 640, 480, defwinflags);
//	//memset(client->texture, 0xffffff, 640 * 480 * 4);
//	//memcpy(client->texture, p, 640 * 480 * 4);
//	set_node_target(client);
//	draw_texture_pix(p, 0, 0, 640, 480);
//	set_pix_target(buf);


	u8 *scaled = stbi_load_from_memory(scaled_image, sizeof scaled_image, &iw, &ih, &ic, 4);
	bgr_to_rgb(scaled, iw, ih);
	node *imgviewer = window("Image Viewer", 200, 300, iw, ih, defwinflags);
	imgviewer->flags &= ~W_visible;
	set_pix_target(imgviewer->texture);
	draw_width = iw;
	draw_texture_pix((u32 *)scaled, 0, 0, iw, ih);
	draw_width = width;



	node *fontviewer = window("stb_truetype", 300, 100, 512, 512, defwinflags);
	fontviewer->flags &= ~W_visible;
	stbtt_fontinfo font;
	u8 *fontbitmap = mmap2(fontviewer->rec.w * fontviewer->rec.h);
	int bitmapw = fontviewer->rec.w;
	if (!stbtt_InitFont(&font, font_file, 0))
	{
		lykos_exit();
	}
	set_node_target(fontviewer);
	draw_background(fontviewer, dark_background);
	char *text = "test again\nnewline";
	u8 *bitmap = make_bitmap();
	g8bpp_to_32bpp(fontviewer->texture, bitmap, fontviewer->rec.w, fontviewer->rec.h);
	//text_draw_string(text, 0, 0, &font);
	//fix
//	int line_height = 18;
//	float fscale = stbtt_ScaleForPixelHeight(&font, line_height);
//	int ascent, descent, linegap;
//	stbtt_GetFontVMetrics(&font, &ascent, &descent, &linegap);
//	//*ascent - *descent + *lineGap
//	//char *word = "Unicode: año Straße, €100";
//	//int omega = 0x03A9;
//	//int word[1] = {omega};
//	//char *word = "This is a test using stb_truetype.\n Newline";
//	char *word = "#include <stdio.h>\n\nint main(void) {\n    printf(\"hello\");\n}\nmoretext\nmoretext\noretext";
//
//	int i, x;
//	set_node_target(fontviewer);
//	draw_background(fontviewer, dark_background);
//	ascent = roundf(ascent * fscale);
//	descent = roundf(descent * fscale);
//	int txt_y = ascent;
//	for (i = 0, x = 0; i < strlen(word); i++) {
//		if (word[i] == '\n') {
//			txt_y += ascent -  descent + linegap;
//			x = 0;
//			continue;
//		}
//		int ax;
//		int lsb;
//		stbtt_GetCodepointHMetrics(&font, word[i], &ax, &lsb);
//
//		int c_x1, c_y1, c_x2, c_y2;
//		stbtt_GetCodepointBitmapBox(&font, word[i], fscale, fscale, &c_x1, &c_y1, &c_x2, &c_y2);
//
//		int y = txt_y + c_y1;
//
//		int byteOffset = x + roundf(lsb * fscale) + (y * bitmapw);
//		stbtt_MakeCodepointBitmap(&font, (u8 *) fontbitmap + byteOffset, c_x2 - c_x1, c_y2 - c_y1, bitmapw, fscale, fscale, word[i]);
//
//		x += roundf(ax * fscale);
//
//		int kern;
//		kern = stbtt_GetCodepointKernAdvance(&font, word[i], word[i + 1]);
//		x += roundf(kern * fscale);
//	}
//	//bgr_to_rgb((u8 *) fontviewer->texture, fontviewer->rec.w, fontviewer->rec.h);
//	g8bpp_to_32bpp(fontviewer->texture, fontbitmap, fontviewer->rec.w, fontviewer->rec.h);
////	for (int k = 0; k < 100 * 100; k++) {
////		fontviewer->texture[k] = 0xfffffffff;
////	}
//
	set_pix_target(buf);



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

	
	node *win3 = window("3D", 200, 300, 500, 300, defwinflags);
	node *win = window("mterm", 100, 100, 500, 300, defwinflags);
	node *win2 = window("xd", 400, 600, 500, 300, defwinflags);
	node *bar = window("bar", 0, 0, width, 20, W_visible | N_title);
	node *launcher = window("launcher", width / 2 - 250, height / 2 - 10, 500, 20, W_visible | N_title | W_draw_border | W_focusable);
	win3->flags &= ~W_visible;
	win->flags &= ~W_visible;
	win2->flags &= ~W_visible;
	MailboxMessage out;
	wm_msg msg;
	int i, j;
	for (;;) {
		while (mbox_receive(mboxid, &out)) {
			//if (valid_msg)
			node *client;
			msg = *(wm_msg *)out.data;
			switch (msg.type) {
			case WM_open:
				handle_open(&msg);
				break;
			case WM_close:
				client = get_window_by_id(msg.window_id);
				if (!client) break;
				remove_window(client);
				break;
			case WM_commit:
				client = get_window_by_id(msg.window_id);
				if (!client) break;
				uint32_t *cbuf = client->shared_buf;

				set_node_target(client);
				draw_texture_pix(cbuf, 0, 0, client->rec.w, client->rec.h);
				set_pix_target(buf);
				break;
			default: break;
			}
		}
		//for (int i = 0; i < width*height; i++) {
		//	buf[i] = BG;
		//}
		vector2 diff = {0};
		KeyEvent ev;
		char evbuf[64] = {0};
		int evbufi = 0;
		while (1) {
			i64 ret = get_key_event(&ev);
			//i64 ret = 0;
			if (ret == 0) {
				break;
			} else if (ev.key == KEY_UP_ARROW) {
				diff.y -= 10;
			} else if (ev.key == '\t') {
				if (focused_window == -1) focused_window = 0;
				send_msg(windows[focused_window], WM_unfocus);
				int i;
				bool found = false;
				for (i = 0; i < win_c; i++) {
					focused_window++;
					if (focused_window > win_c - 1) focused_window = 0;
					node *win = windows[focused_window];
					if (!(win->flags & W_focusable) || !(win->flags & W_visible)) {
						continue;
					} else {
						found = true;
						break;
					}
				}
				if (!found) focused_window = -1;
				else send_msg(windows[focused_window], WM_focus);
			} else if (ev.key == 27) {
				lykos_exit();
			} else if (ev.key == 'q' && ev.modifiers & MOD_CTRL) {
				if (windows[focused_window]) {
					node *win = windows[focused_window];
					if (win->flags & W_visible) {
						win->flags &= ~W_visible;
					} else {
						win->flags |= W_visible;
					}
					remove_window(win);
					send_msg(win, WM_close);
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
			} else if (ev.key >= '!' && ev.key <= '~' || ev.key == '\n') {
				if (evbufi < sizeof evbuf) {
					evbuf[evbufi++] = ev.key;
				}
			}
		}

		if (launcher->flags & W_visible) {
			set_node_target(launcher);
			draw_background(launcher, WHITE);
			node *n;
			n = text_input(3, launcher, "", launcher->rec, N_focused);
			int x = 0;
			j = 0;
			char c;
			while (c = n->buffer[j++]) {
				if (c == '\n') {
					n->buffer[strlen(n->buffer) - 2] = '\0';
					exec(n->buffer);
					memset(n->buffer, 0, sizeof n->buffer);
					n->buff_i = 0;
				}
				draw_char_scaled(c, x, 5, BLACK, 1);
				x += 8;
			}
			set_pix_target(buf);
		}
		if (win3->flags & W_visible)
		{
			set_node_target(win3);
			draw_background(win3, dark_background);
			render3d();
			set_pix_target(buf);
		}

		if (win->flags & W_visible)
		{
			set_node_target(win);
			draw_background(win, dark_background);
			char *prompt = "/user> ";
			int prompt_len = strlen(prompt) * 8;
			//draw_string(prompt, 0, 0, WHITE);
			//text_draw_string(prompt, 0, 0, &font);
			char *a;
			node *n;
			n = text_input(1, win, "", win->rec, N_focused);
			a = n->buffer;

			int x = prompt_len;
			int y = 0;
			//text_draw_string(a, x, 0, &font);
			//while (*a) {
//					draw_char_scaled(*a, x, y, WHITE, 1);
//					x += 8;
//				}
//				*a++;
			//}
			set_pix_target(buf);
		}

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

		if (focused_window >= 0) {
			node *focuswin = windows[focused_window];
			if (focuswin->flags & W_movable) {
				windows[focused_window]->rec.x += diff.x;
				windows[focused_window]->rec.y += diff.y;
			}
		}
		imgviewer->rec.x += 1;
		imgviewer->rec.y += 1;
		//set_focus(fontviewer);

		//clientwins[0]->rec.x += 5;
		
		set_node_target(bar);
		draw_background(bar, WHITE);
		draw_string("Applications File Edit View", 5, 5, BLACK);
		if (focused_window >= 0)
			draw_string(windows[focused_window]->title, 30 * 8 , 5, BLACK);
		set_pix_target(buf);

		for (i = 0; i < node_c; i++) {
			node *np = &nodes[i];
			bool has_focus = np->parent == windows[focused_window];
			if (has_focus && np->flags & N_text && evbufi) {
				for (int x = 0; x < evbufi; x++) {
					np->buffer[np->buff_i++] = evbuf[x];
				}
			}
		}
		//fixme drawstack
		for (i = 0; i < win_c; i++) {
			node *np = windows[i];
			if (!(np->flags & W_visible))
				continue;

			draw_texture(np);
			if (np->flags & W_draw_decoration)
				draw_decoration(np);
			if (np->flags & W_draw_border)
				draw_border(np);
		}

		if (focused_window >= 0) {
			node *np = windows[focused_window];
			if (np->flags & W_visible) {
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
