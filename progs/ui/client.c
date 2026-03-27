#include "syscalls.h"
#include "protocol.h"
#include <string.h>

int main(void) {
	//u64 sz;
	//u32 *screen = shm_map(0, &sz);
	//if (!screen) lykos_exit();

	//memset(screen, 0xffffff, 640 * 480 * 4);
	
	int mboxid = 1;
	int err = mbox_create(mboxid); 
	if (err < 0) {
		lykos_exit();
	}

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
	while (1) {
		while (mbox_receive(mboxid, &out) < 0);
		msg = *(wm_msg *) out.data;
		if (msg.type != WM_ok) continue;
		write("hello");
	}

	size_t sz;
	shared_buffer *buf = shm_map(msg.shm_id, &sz);
	int i;
	for (i = 0; i < 300*300 * 4; i++) {
		buf->surface[i] = 0xff0000;
	}
	buf->commited = 1;
	return 0;
}
