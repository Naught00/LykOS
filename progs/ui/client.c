#include "syscalls.h"
#include <string.h>

int main(void) {
	u64 sz;
	u32 *screen = shm_map(0, &sz);
	if (!screen) lykos_exit();

	memset(screen, 0xffffff, 640 * 480 * 4);
	//int i;
	//for (i = 0; i < 640 * 480; i++) {
	//	screen[i] = 0xfffffffffffff;
	//}
	return 0;
}
