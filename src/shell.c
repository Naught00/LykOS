#include "shell.h"
#include "arch/x86_64/util.h"
#include "drivers/pit.h"
#include "drivers/serial.h"
#include "fs/vfs.h"
#include "graphics/draw.h"
#include "klib/kstring.h"
#include "klib/time.h"
#include "mem/kalloc.h"
#include "mem/mem.h"
#include "proc/task.h"
#include "req.h"
#include "user/elf.h"
// #define STB_DS_IMPLEMENTATION
// #define STBDS_REALLOC(context, ptr, size) krealloc(ptr, size)
// #define STBDS_FREE(context, ptr) kfree(ptr)
#include "terminal.h"
#include "user/exec.h"
#include "user/input.h"
#include "vendor/stb_ds.h"
#include <stdbool.h>
#include <stdint.h>

static void print_help(void) {

  terminal_fstring("Command list...\n"
                   " ls             - lists files in current directory\n"
                   " exec $filename - executes ELF file               \n"
                   " cat $filename  - prints file contents to screen  \n"
                   " logo           - redraws screen with logo)       \n"
                   " mmap           - prints memory map information   \n"
                   " memstats       - prints memory allocation stats\n"
                   " dbgalloc       - allocates 1 frame\n"
                   " tasks          - shows currently running tasks\n"
                   " freelist       - prints kallocs freelist \n");
}
// max length of a path is 256
char cwd_buf[256];
kstring cwd = KSTRING(cwd_buf, 256);

static int8_t change_dir(kstring *path) {
  if (path->len > cwd.cap) {
    serial_fstring("Path length {uint} is bigger than {uint}\n", path->len,
                   cwd.cap);
    return -1;
  }

  memcpy(cwd.buf, path->buf, path->len);
  cwd.len = path->len;
  return 0;
}

static void print_cwd(void) {
  serial_fstring("{str}\n", cwd.buf);
  terminal_fstring("{str}\n", cwd.buf);
}

static int cat_file(kstring *path) {
  u64 file_size = 0;
  char *file_buf = read_file(path, &file_size);
  if (file_buf == NULL)
    return -1;
  for (u32 i = 0; i < file_size; i++) {
    terminal_fstring("{char}", file_buf[i]);
  }
  kfree(file_buf);
  return 0;
}

// assumes first word is the command
// args are space seperated
typedef struct {
  kstring args[16];
  u8 count;
} args_list;

bool parse_args(kstring *line, args_list *out) {
  out->count = 0;
  u16 i = 0;

  // skip first word and spaces, i=start of 2nd word
  while (i < line->len && line->buf[i] != ' ') {
    i++;
  }
  while (i < line->len && line->buf[i] == ' ') {
    i++;
  }

  while (i < line->len && out->count < 16) {
    while (i < line->len && line->buf[i] == ' ') {
      i++;
    }

    if (i >= line->len)
      break;

    u16 start = i;

    while (i < line->len && line->buf[i] != ' ') {
      i++;
    }

    out->args[out->count].buf = &line->buf[start];
    out->args[out->count].len = i - start;
    out->count++;
  }

  return out->count > 0;
}

void shell_init(void) {
  char init_path_buf[256];
  kstring init_path = KSTRING(init_path_buf, 256);
  APPEND_STR(&init_path, "/");
  if (init_path.error)
    serial_fstring("Error setting initial path\n");

  if (change_dir(&init_path) != 0)
    serial_fstring("Error setting initial path\n");
}

void shell_execute(kstring *line) {
  args_list args;
  parse_args(line, &args);

  u32 temp_color = term_color;
  term_color = WHITE;
  term_ypos += 8 * g_scale;
  term_xpos = 0;

  if (kstrncmp(line, "cd", 2)) {
    if (!parse_args(line, &args)) {
      terminal_fstring("Path missing\n");
      goto skip_history;
    }

    kstring path = args.args[0];
    bool is_valid = valid_path(&path);
    if (path.buf[0] != '/')
      ; // relative, append cwd
    else
      ;

    char *arr = NULL;

  } else if (kstrncmp(line, "ls", 2))
    list_files(&cwd);

  else if (kstrncmp(line, "cat", 3)) {
    if (!parse_args(line, &args)) {
      terminal_fstring("File needed to cat\n");
      goto skip_history;
    }

    kstring path = args.args[0];
    cat_file(&path);

  } else if (kstrncmp(line, "exec", 4)) {
    if (!parse_args(line, &args)) {
      terminal_fstring("File needed to exec\n");
      goto skip_history;
    }

    // storing this inside arena/pool or something with a task would help kepe
    // track for freeing it, can't create the name on the stack since it will be
    // invalid as task lives longer than this stack frame, for now its just
    // leaked
    kstring path = args.args[0];
    char *name_buf = kalloc(path.len + 1);
    memcpy(name_buf, path.buf, path.len);
    name_buf[path.len] = '\0';

    i64 ret = exec(name_buf);
    if (ret == -1) {
      terminal_fstring("Could not find file\n");
      goto skip_history;
    } else if (ret == -2) {
      terminal_fstring("Could not exec file\n");
      goto skip_history;
    } else {
      current_input_target = USERSPACE;
      while (tasks[ret].state != TASK_DEAD) {
        yield();
      }
      current_input_target = KERNEL_TERMINAL;
    }
  }

  else if (kstrncmp(line, "ascii", 5))
    draw_ascii();

  else if (kstrncmp(line, "logo", 4))
    draw_logo();

  else if (kstrncmp(line, "gfx", 3))
    debug_graphics();

  else if (kstrncmp(line, "exit", 4))
    qemu_shutdown();

  else if (kstrncmp(line, "pwd", 3))
    print_cwd();

  else if (kstrncmp(line, "rainbow", 7))
    infinite_rainbow(get_framebuffer());

  else if (kstrncmp(line, "red", 3))
    term_color = RED;

  else if (kstrncmp(line, "blue", 4))
    term_color = BLUE;

  else if (kstrncmp(line, "green", 5))
    term_color = GREEN;

  else if (kstrncmp(line, "mmap", 4))
    print_memmap();

  else if (kstrncmp(line, "help", 4))
    print_help();

  else if (kstrncmp(line, "allocstats", 4))
    print_allocation_stats();

  else if (kstrncmp(line, "paging", 6))
    print_paging();

  else if (kstrncmp(line, "memstats", 8))
    print_memory_stats();

  else if (kstrncmp(line, "dbgalloc", 8))
    pmm_alloc_frame();

  else if (kstrncmp(line, "clear", 5))
    terminal_clearscreen();

  else if (kstrncmp(line, "freelist", 8))
    debug_freelist();

  else if (kstrncmp(line, "tasks", 5))
    debug_tasks();

  else if (kstrncmp(line, "ms", 2))
    terminal_fstring("{uint}\n", ticks_to_ms(pit_get_ticks()));
  else {
    terminal_fstring("Unknown command\n");
    goto skip_history;
  }

  history_add(line);

skip_history:
  term_color = temp_color;
  terminal_newline();
  return;
}
