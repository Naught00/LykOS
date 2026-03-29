#include <stdint.h>

#define MAX_TITLE 32
#define DISPLAY 0
enum wm_msg_type {
	WM_open,
	WM_ok,
	WM_close,
	WM_commit,
	WM_focus,
	WM_unfocus,
};

enum window_flags {
	W_visible = 0x1,
	W_draw_decoration = 0x2,
	W_draw_border = 0x4,
	W_focusable = 0x8,
};

typedef struct wm_msg {
	enum wm_msg_type type;
	int window_id;
	union {
		//WM_open
		struct {
			char title[MAX_TITLE];
			int x;
			int y;
			int w;
			int h;
			unsigned int flags;
			int mailbox;
		};
		//WM_ok
		struct {
			int shm_id;
		};
	};
} wm_msg;
