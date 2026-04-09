#pragma once
#include "graphics.c"

window *mwmu_render_target;
window *mwmu_last_render_target;
void set_render_target(window *win) {
	mwmu_last_render_target = mwmu_render_target;
	mwmu_render_target = win;

	draw_width  = win->rec.w;
	draw_height = win->rec.h;
	pixels      = win->surface;
	return;
}

void pop_render_target() {
	if (mwmu_last_render_target) {
		set_render_target(mwmu_last_render_target);
	}
}
