#include "mwm/client.c"
#include "graphics.c"
#include "text.c"

int main(void) {
	window *win = open_window("launcher", -1, -1, 500, 40, W_visible | W_draw_border | W_focusable);
	set_render_target(win);
	draw_background(WHITE);
	commit_win(win);

	char key;
	KeyEvent key_event;
	stack(char, 256) program_name = {0};
	bool should_clear_text_on_next_press = false;
	while (!should_close(win)) {
		check_messages();
		draw_background(WHITE);

		while (key_events(win)) {
			if (should_clear_text_on_next_press) {
				stack_reset(program_name);
				should_clear_text_on_next_press = false;
			}

			key_event = next_key(win);
			key = key_event.key;
			if (key >= ' ' && key <= '~') {
				push(program_name, key);
			} else if (key == '\n') {
				int ret = exec(program_name.stack);
				if (ret < 0) {
					strcpy(program_name.stack, "Could not find program");
					should_clear_text_on_next_press = true;
				} else {
					close_window(win);
					return 0;
				}
			}
		}

		draw_text(program_name.stack, true, 5, 5);
		commit_win(win);
		sleep(16);
	}
}
