typedef struct color {
	uint8_t r, g, b, a;
} color;

color RED = {255, 0, 0, 255};

typedef struct rectangle {
	int x, y, w, h;
} rectangle;
int draw_width;
int draw_height;
#define BPP 4
uint32_t *pixels;

void clear_screen(color c) {
//	memset(pixels, *(uint32_t *) &c, draw_width * draw_height * BPP);
}

void set_render_target(node *win) {
	draw_width = win->rec.w;
	draw_width = win->rec.h;
	pixels = win->surface;
	return;
}

