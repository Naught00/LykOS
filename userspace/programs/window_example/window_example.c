/* Window manager client example. 
   Opens window and draws a red background 
   The client must sleep between frames to avoid
   taking up too much CPU time from the window manager.
*/


#include "shapes.h"
#include "mwm/client.c"
#include "graphics.c"

int main() {
	window *win;
	win = open_window("test", 50, 100, 300, 300, -1);

	while (!should_close(win)) {
		set_render_target(win);
		draw_background(RED);
		commit_win(win);

		check_messages();
		sleep(16);
	}
	close_window(win);
	return 0;
}
