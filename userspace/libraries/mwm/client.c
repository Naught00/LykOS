#ifndef MWM_CLIENT
#define MWM_CLIENT

#include <stdint.h>
#include "shapes.h"
#include "lykosapi.h"
#include "protocol.h"
#include "basic.h"


enum local_flags {
	WC_should_close = 0x1,
	WC_has_focus    = 0x2,
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
int DEFWINFLAGS = W_visible | W_draw_decoration | W_draw_border | W_focusable | W_movable;

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
	//FIXME sleep on rec
	while (1) {
		while (!mbox_receive(mboxid, &out)) {
			sleep(1);
			continue;
		}

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

void close_window(window *win) {
	wm_msg msg;
	msg.type = WM_close;
	msg.window_id = win->window_id;
	mbox_send(DISPLAY, &msg, sizeof(msg));
	win->local_flags |= WC_should_close;
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

u64 keys[2];
u64 ascii_to_key(char c) {
	u64 i = c - '!';
	return 1 << i;
}

bool is_key_pressed(char k) {
	if (k < '!' || k > '~') return false;

	int index = 0;
	u32 key_bit;
	key_bit = ascii_to_key(k);
	if (k > (1 << 64)) index = 1;

	if (keys[index] & key_bit) {
		return true;
	} else {
		return false;
	}
}

void poll_keys(window *) {
}

void check_messages() {
	wm_msg msg;
	MailboxMessage out;
	while (mbox_receive(mboxid, &out)) {
		msg = *(wm_msg *) out.data;
		//todo
		//if (valid_msg) 
			window *win = get_window_by_win_id(msg.window_id);
		switch (msg.type) {
		case WM_close:
			win->local_flags |= WC_should_close;
			break;
		case WM_focus:
			win->local_flags |= WC_has_focus;
			break;
		case WM_unfocus:
			win->local_flags &= ~WC_has_focus;
			break;
		default: break;
		}
	}

	int i;
	for (i = 0; i < nodes.sp; i++) {
		window *win = &stack_index(nodes, i);
		if (win->local_flags & WC_has_focus) {
			poll_keys(win);
		}
	}
	return;
}


#define should_close(win) (win->local_flags & WC_should_close)
#define has_focus(win) (win->local_flags & WC_has_focus)
#endif
