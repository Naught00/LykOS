#define MAX_TITLE 32
enum wm_msg_type {
	WM_open,
	WM_ok,
	WM_close
};

enum window_flags {
	W_visible = 0x1,
	W_draw_decoration = 0x2,
	W_draw_border = 0x4,
	W_focusable = 0x8,
};

typedef struct wm_msg {
	enum wm_msg_type type;
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

typedef struct shared_buffer {
	_Atomic u32 commited;
	u32 surface[];
} shared_buffer;
