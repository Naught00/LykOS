/* Window manager client example. 
   Opens window and draws a red background 
   The client must sleep between frames to avoid
   taking up too much CPU time from the window manager.
*/


#include "shapes.h"
#include "mwm/client.c"
#include "graphics.c"
#include "text.c"

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
	u8 *bitmap = load_font_mem(font_file);
	sleep(0);

	int len = strlen(source_file);
	int head = 0;
	char line[256] = {0};
	while (!should_close(win)) {
		set_render_target(win);
		draw_background(BLACK);
		int i, j;
		KeyEvent ev;
		while (1) {
			i64 ret = get_key_event(&ev);
			if (ret == 0) {
				break;
			} else if (ev.key == 'j') {
				head += 1;
			} else if (ev.key == 'k') {
				head -= 1;
			} else if (ev.key == 'q') {
				write("Q PRESSED!\n");
			}
		}
		float x = 0, y = 0;
		int line_index = 0;
		for (i = 0, j = 0; i < len; i++) {
			if (line_index >= head) line[j++] = source_file[i];
			else continue;
			if (source_file[i] == '\n') {
				line_index++;
				draw_text(bitmap, line, false, &x, &y);
				j = 0;
				memset(line, 0, sizeof line);
				continue;
			}
		}

		commit_win(win);
		sleep(16);
		check_messages();
	}
	close_window(win);
	return 0;
}
