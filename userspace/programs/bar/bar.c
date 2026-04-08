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

	log_printf("clearing background...");
	log_printf("subscribing to WM_change_focus messages...");
	subscribe(WM_change_focus);
	log_printf("blocking...");

	char title[MAX_TITLE] = {0};
	while (!should_close(win)) {
		check_messages_block();
		log_printf("unblocked on message...");
		char *new_title = focus_changed();
		if (new_title) {
			strncpy(title, new_title, sizeof title);
			log_printf("got new title");
		}
		set_render_target(win);
		draw_background(WHITE);
		float x = 5;
		float y = 0;
		draw_text_pro("Programs File Edit View  ", true, &x, &y);
		draw_text_pro(title, true, &x, &y);
		commit_win(win);

	}
	return 0;
}
