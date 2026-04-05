/* Window manager client example program.
   Opens window and prints its source code to the screen.
   The client must sleep between frames to avoid
   taking up too much CPU time from the window manager.
*/


#include "shapes.h"
#include "mwm/client.c"
#include "graphics.c"
#include "text.c"
#include "stdio.h"

u8 font_file[] = {
#embed "../../fonts/mono.ttf"
};

char source_file[] = {
#embed "window_example.c"
};

char *str = "this is a test with text\n";

int main() {
	window *win;
	win = open_window("test", -1, -1, 640, 480, -1);
	if (!win) lykos_exit();

	int i, j;
	int len = strlen(source_file);
	int linec = 0;
	for (i = 0; i < len; i++) {
		if (source_file[i] == '\n') {
			linec++;
		}
	}

	int  head = 0;
	char line[256] = {0};
	set_font(MONO);
	bool redraw = true;
	while (!should_close(win)) {
		check_messages();
		KeyEvent key_event;
		while (key_events(win)) {
			key_event = next_key(win);
			char key = key_event.key;
			switch (key) {
			case 'j':
				if (head < linec) head += 1;
				break;
			case 'k':
				if (head >= 0) head -= 1;
				break;
			default:
				printf("%s: Pressed %c\n", __FILE__, key);
				break;
			}
		}
		set_render_target(win);
		draw_background(BLACK);
		float x = 0, y = 0;
		int line_index = 0;
		for (i = 0, j = 0; i < len; i++) {
			if (line_index >= head) line[j++] = source_file[i];
			if (source_file[i] == '\n') {
				line_index++;
				if (line_index <= head) continue;

				draw_text_pro(line, false, &x, &y);
				j = 0;
				memset(line, 0, sizeof line);
				continue;
			}
		}

		commit_win(win);
		sleep(16);
	}
	close_window(win);
	return 0;
}
