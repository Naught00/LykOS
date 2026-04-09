#include <stdlib.h>
#define STBI_NO_STDIO
#define STBI_NO_THREAD_LOCALS
#define STB_IMAGE_IMPLEMENTATION
#include "vendor/stb_image.h"
#include "mwm/client.c"
#include "mwm/utils.c"
#include "graphics.c"

u8 img_buffer[] = {
#embed "galaxy-cropped.jpg"
};

int main(void) {
	int iw, ih, ic;

	u8 *image = stbi_load_from_memory(img_buffer, sizeof img_buffer, &iw, &ih, &ic, 4);
	bgr_to_rgb(image, iw, ih);
	window *win = open_window("background", 0, 0, iw, ih, W_visible | W_background);
	set_render_target(win);
	draw_texture_pix((u32 *) image, 0, 0, iw, ih);
	commit_win(win);
	return 0;
}

