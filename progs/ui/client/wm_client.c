#include <stdint.h>
#include "shapes.h"
#include "syscalls.h"
#include "protocol.h"
#include "basic.h"


enum local_flags {
	WC_should_close = 0x1,
};

typedef struct window window;
struct window {
	int id;
	struct {
		char *title;
		int  window_id;
	};
	rectangle rec;
	volatile uint32_t *surface;
	unsigned int flags;
	unsigned int local_flags;
};

stack(window, 20) nodes;

atomic bool init = false;
int mboxid;
int DEFWINFLAGS = W_visible | W_draw_decoration | W_draw_border | W_focusable;

void init_client() {
	mboxid = mbox_create(-1); 
	if (mboxid < 0) lykos_exit();
	init = true;
	return;
}

window *open_window(char *title, int x, int y, int w, int h, unsigned int flags) {
	size_t sz;
	MailboxMessage out;
	wm_msg msg;
	window *np;

	if (!init) {
		init_client();
	}

	if (flags == -1) {
		flags = DEFWINFLAGS;
	}

	msg.type = WM_open;
	//?
	if (strlen(title) > MAX_TITLE) return null;
	strcpy(msg.title, title);

	msg.x = x;
	msg.y = y;
	msg.w = w;
	msg.h = h;
	msg.flags = flags;
	msg.mailbox = mboxid;
	mbox_send(0, &msg, sizeof(msg));

	np = &stack_next(nodes);
	np->id = nodes.sp - 1;
	np->title = title;
	//np->window_id = win_c;
	np->rec = (rectangle){x, y, w, h};
	np->flags = flags;
	np->local_flags = 0;
	//windows[win_c++] = np;

	memset(&msg, 0, sizeof msg);
	while (1) {
		while (!mbox_receive(mboxid, &out));
		msg = *(wm_msg *) out.data;
		if (msg.type == WM_ok) {
			np->window_id = msg.window_id;
			uint32_t *surface = shm_map(msg.shm_id, &sz);
			np->surface = surface;
			break;
		}
	}

	return np;
}

void commit_win(window *win) {
	wm_msg msg;
	msg.type = WM_commit;
	msg.window_id = win->window_id;
	mbox_send(DISPLAY, &msg, sizeof(msg));
	return;
}

window *get_window_by_win_id(int win_id) {
	int i;
	for (i = 0; i < nodes.sp; i++) {
		window *n = &stack_index(nodes, i);
		if (n->window_id == win_id) 
			return n;
	}
	return null;
}

void check_messages() {
	wm_msg msg;
	MailboxMessage out;
	while (mbox_receive(mboxid, &out)) {
		msg = *(wm_msg *) out.data;
		switch (msg.type) {
		case WM_close:
			write("got close\n");
			window *win = get_window_by_win_id(msg.window_id);
			win->local_flags |= WC_should_close;
			break;
		default: break;
		}
	}
}

#define should_close(win) (win->local_flags & WC_should_close)
