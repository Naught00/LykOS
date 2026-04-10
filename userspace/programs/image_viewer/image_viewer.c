#include <stdlib.h>
#include "vendor/stb_image.h"
#include "mwm/client.c"
#include "mwm/utils.c"
#include "graphics.c"

u8 img_buffer[] = {
#embed "scaled_image.jpg"
};

int main(void) {
	int iw, ih, ic;
	u8 *image = stbi_load_from_memory(img_buffer, sizeof img_buffer, &iw, &ih, &ic, 4);
	bgr_to_rgb(image, iw, ih);
	window *win = open_window("Image Viewer", -1, -1, iw, ih, -1);
	while (!should_close(win)) {
		set_render_target(win);
		draw_texture_pix((u32 *) image, 0, 0, iw, ih);
		commit_win(win);
		check_messages_block();
	}
	return 0;
}

