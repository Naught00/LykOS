#ifndef GRAPHICS_C
#define GRAPHICS_C
#include <math.h>
#include "shapes.h"
#include "mwm/client.c"

typedef struct color {
	uint8_t r, g, b, a;
} color;

color RED = {0, 0, 255, 255};
color WHITE = {255, 255, 255, 255};
color BLACK = {0, 0, 0, 255};

int draw_width;
int draw_height;
#define BPP 4
volatile uint32_t *pixels;

void set_render_target_pix(volatile u32 *buf, int width, int height) {
	draw_width  = width;
	draw_height = height;
	pixels      = buf;
	return;
}

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

bool in_rectangle(int x, int y, rectangle r) {
	if (x >= r.x && x <= r.x + r.w && y >= r.y && y <= r.y + r.h) return true;
	else return false;
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
	//fixme : volatile memcpy?
//	bool same_size = w == draw_width && h == draw_height;
//	if (same_size) {
//		memcpy(pixels, texture, draw_width * draw_height * sizeof(u32));
//		return;
//	}
	manual_draw_texture_pix(texture, ox, oy, w, h);
	return;
}

void draw_background(color c) {
	int i;
	for (i = 0; i < draw_width * draw_height; i++) {
		pixels[i] = *(uint32_t *) &c;
	}
}

vector2 to_screen(vector2f p) {
	vector2 vec;
	vec.x = (p.x + 1)/2 * draw_width;
	vec.y = (1 - (p.y + 1)/2) * draw_height;
	return vec;
}

vector2f project(vector3f p) {
	vector2f vec;
	vec.x = p.x / p.z;
	vec.y = p.y / p.z;
	return vec;
}

vector3f translate_z(vector3f p) {
	p.z += 1.0;
	return p;
}

vector3f rotate_xz(vector3f p, float angle) {
	float c = cosf(angle);
	float s = sinf(angle);
	vector3f v = {p.x * c - p.z * s, p.y, p.x * s + p.z * c};
	return v;
}


window *render_target;
window *last_render_target;
//depends on windowing system
void set_render_target(window *win) {
	last_render_target = render_target;
	render_target = win;
	draw_width = win->rec.w;
	draw_height = win->rec.h;
	pixels     = win->surface;
	return;
}

void pop_render_target() {
	if (last_render_target) {
		set_render_target(last_render_target);
	}
}
#endif
