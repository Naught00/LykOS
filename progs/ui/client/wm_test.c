#include "shapes.h"
#include "wm_client.c"
#include "graphics.c"

int main() {
	window *win;
	win = open_window("test", 50, 100, 300, 300, -1);

	while (1) {
		check_messages();

		set_render_target(win);
		draw_background(RED);
		commit_win(win);

		sleep(16);
	}
}
