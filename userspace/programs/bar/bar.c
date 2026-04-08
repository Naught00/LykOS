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
	printf("[bar] subscribing to WM_change_focus messages...\n");
	subscribe(WM_change_focus);
	printf("[bar] blocking...\n");

	char title[MAX_TITLE] = {0};
	while (!should_close(win)) {
		check_messages_block();
		printf("[bar] unblocked on message...\n");
		char *new_title = focus_changed();
		if (new_title) {
			strncpy(title, new_title, sizeof title);
			printf("got new title\n");
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
