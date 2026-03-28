#include "wm_client.c"

int main() {
	node *win;
	win = window("test", 50, 100, 300, 300, -1);

	while (1) {
		check_messages();
		int i;
		for (i = 0; i < 300 * 300; i++) {
			win->surface[i] = 0xff0000;
		}
		commit_win(win);
		sleep(16);
	}
}
