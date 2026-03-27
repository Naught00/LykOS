#include "syscalls.h"
#include "protocol.h"
#include <string.h>

struct color {
	uint8_t b, g, r, a;
};

int main(void) {
	//u64 sz;
	//u32 *screen = shm_map(0, &sz);
	//if (!screen) lykos_exit();

	//memset(screen, 0xffffff, 640 * 480 * 4);
	
	int mboxid = mbox_create(-1); 
	if (mboxid < 0)
		lykos_exit();

	wm_msg msg;
	//memset(msg.title, 0, sizeof msg.title);
	msg.type = WM_open;
	strcpy(msg.title, "Test Windowff");

	msg.x = 100;
	msg.y = 200;
	msg.w = 300;
	msg.h = 300;
	msg.mailbox = mboxid;

	msg.flags = W_visible | W_draw_decoration | W_draw_border;

	mbox_send(0, &msg, sizeof(wm_msg));
	MailboxMessage out;
	size_t sz;
	shared_buffer *buf = NULL;
	struct color c = {0, 0, 0, 0};
	bool up = true;
	int step = 10;
	while (1) {
		memset(&msg, 0, sizeof msg);
		while (mbox_receive(mboxid, &out));
		msg = *(wm_msg *) out.data;
		switch (msg.type) {
		case WM_ok:
			buf = shm_map(msg.shm_id, &sz);
			break;
		}

		if (!buf) continue;

		buf->commited = 0;
		for (int i = 0; i < 300*300; i++) {
			buf->surface[i] = *(u32 *) &c;
		}
		buf->commited = 1;

		if (c.b >= 255)
			up = false;
		else if (c.b <= 0) up = true;

		if (up) {
			c.r += step;
			c.b += step * 2;
		} else {
			c.r -= step;
			c.b -= step * 2;
		}
		sleep(16);
	}
	return 0;
}
