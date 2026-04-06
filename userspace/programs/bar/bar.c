#include "mwm/client.c"
#include "graphics.c"
#include "text.c"

u8 font_file[] = {
#embed "../../fonts/regular.ttf"
};

int main(void) {
	window *win = open_window("bar", 0, 0, -1, 25, W_visible);
	set_render_target(win);
	draw_background(WHITE);
	commit_win(win);

	printf("[bar] clearing background...\n");
	printf("[bar] going to sleep...\n");

	while (!should_close(win)) {
		check_messages();
		set_render_target(win);
		draw_background(WHITE);
		draw_text("Applications File Edit View", true, 5, 0);
		commit_win(win);
		sleep(1000);
	}
	return 0;
}
