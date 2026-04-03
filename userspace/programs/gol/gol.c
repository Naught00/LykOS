#include "shapes.h"
#include "mwm/client.c"
#include "graphics.c"
#include "text.c"

#define GRID_W 800
#define GRID_H 800
#define GRID_SIZE 100
#define RECT_SIZE (GRID_W / GRID_SIZE)

typedef struct {
    rectangle r;
    int alive;
} cell;

cell grid[GRID_SIZE][GRID_SIZE];

// color WHITE = {255, 255, 255, 255};
// color BLACK = {0, 0, 0, 255};

void draw_grid(void) {
    for (int row = 0; row < GRID_SIZE; row++) {
        for (int col = 0; col < GRID_SIZE; col++) {
            if (grid[row][col].alive)
                draw_rectangle(grid[row][col].r, WHITE);
            else
                draw_rectangle(grid[row][col].r, BLACK);
        }
    }
}

u8 font_file[] = {
#embed "../../fonts/mono.ttf"
};

int count_neighbors(int row, int col) {
    int count = 0;
    for (int dr = -1; dr <= 1; dr++) {
        for (int dc = -1; dc <= 1; dc++) {
            if (dr == 0 && dc == 0) continue;
            int nr = row + dr;
            int nc = col + dc;
            if (nr >= 0 && nr < GRID_SIZE && nc >= 0 && nc < GRID_SIZE)
                count += grid[nr][nc].alive;
        }
    }
    return count;
}

void tick(void) {
    static cell next[GRID_SIZE][GRID_SIZE];
    for (int row = 0; row < GRID_SIZE; row++) {
        for (int col = 0; col < GRID_SIZE; col++) {
            next[row][col] = grid[row][col];
            int n = count_neighbors(row, col);
            if (grid[row][col].alive)
                next[row][col].alive = (n == 2 || n == 3);
            else
                next[row][col].alive = (n == 3);
        }
    }
    memcpy(grid, next, sizeof(grid));
}
static unsigned int seed = 12345;

int rand(void) {
    seed = seed * 1103515245 + 12345;
    return (seed >> 16) & 0x7FFF;
}
u8 *bitmap;

char *pop = "pop counter 100\n";

void draw_pop(void){
	static bool swap;
	if (swap) {
		printf(pop);
		swap = false;
	} else {
		printf("wow!, swapping text\n");
		swap = true;
	}
	return;
}

int main() {
    window *win;
    win = open_window("Game Of Life", -1, -1, GRID_W, GRID_H, -1);

    bitmap = load_font_mem(font_file);
    if (!bitmap) {
	    write("HERE\n");
	    close_window(win);
	    lykos_exit();
    }

    for (int row = 0; row < GRID_SIZE; row++) {
        for (int col = 0; col < GRID_SIZE; col++) {
            grid[row][col].r.x = col * RECT_SIZE;
            grid[row][col].r.y = row * RECT_SIZE;
            grid[row][col].r.w = RECT_SIZE;
            grid[row][col].r.h = RECT_SIZE;
            grid[row][col].alive = (rand() % 4) == 0;
        }
    }

    while (!should_close(win)) {
        set_render_target(win);
        draw_background(RED);
        draw_grid();
        tick();
        draw_pop();
        commit_win(win);
        check_messages();
        sleep(300);
    }

    close_window(win);
    return 0;
}
