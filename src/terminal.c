#include "terminal.h"
#include "drivers/ps2.h"
#include "drivers/serial.h"
#include "fs/fat16.h"
#include "graphics/draw.h"
#include "klib/kstring.h"
#include "mem/arena.h"
#include "mem/kalloc.h"
#include "proc/task.h"
#include "req.h" //lsp bug leave included
#include "shell.h"
#include "user/input.h"
#include "vendor/targa.h"
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define DEFAULT_TERMINAL_SCALE 2
// stores input for commands
static char command_buf[256];
static kstring command_content = KSTRING(command_buf, 256);

// term cursor / drawing position
volatile u64 term_xpos = 0;
volatile u64 term_ypos = 0;
u32 term_color;

static void draw_cursor_term(void);
static void draw_kstring_term(kstring *kstr);
static void clear_current_command(void);

static const char *lykos_ascii =
    "       __                  __         ______    ______  \n"
    "      /  |                /  |       /      \\  /      \\ \n"
    "      $$ |       __    __ $$ |   __ /$$$$$$  |/$$$$$$  | \n"
    "      $$ |      /  |  /  |$$ |  /  |$$ |  $$ |$$ \\__$$/ \n"
    "      $$ |      $$ |  $$ |$$ |_/$$/ $$ |  $$ |$$      \\ \n"
    "      $$ |      $$ |  $$ |$$   $$<  $$ |  $$ | $$$$$$  | \n"
    "      $$ |_____ $$ \\__$$ |$$$$$$  \\ $$ \\__$$ |/  \\__$$ | \n"
    "      $$       |$$    $$ |$$ | $$  |$$    $$/ $$    $$/  \n"
    "      $$$$$$$$/  $$$$$$$ |$$/   $$/  $$$$$$/   $$$$$$/   \n"
    "                /  \\__$$ |                               \n"
    "                $$    $$/                                \n"
    "                 $$$$$$/                                 \n";

typedef struct {
  kstring entries[1000];
  u32 count;
  arena arena;
  char *start;
} cmd_history;

cmd_history hist;
static u32 hist_position = 0;

void draw_logo(void) {
  clear_screen(get_framebuffer(), BLACK);
  fat16_dir_entry_t targa_entry = *fat16_find_file("logo.tga");
  char *file_buf = kalloc(targa_entry.file_size);
  fat16_read_file(&targa_entry, file_buf);

  unsigned int *data;
  data = tga_parse((unsigned char *)file_buf, targa_entry.file_size);
  u64 width = data[0];
  u64 height = data[1];

  serial_fstring("W: {uint}, H: {uint}", width, height);

  size_t start_x = 0;
  size_t start_y = 0;
  for (u64 py = 0; py < height; py++) {
    for (u64 px = 0; px < width; px++) {
      // Get pixel from TGA (skip first 2 elements which are width/height)
      u32 pixel = data[2 + (py * width + px)];
      // u32 a = (pixel >> 24) & 0xFF;
      u8 b = (pixel >> 0) & 0xFF;
      u8 g = (pixel >> 8) & 0xFF;
      u8 r = (pixel >> 16) & 0xFF;
      u8 a = (pixel >> 24) & 0xFF;
      u32 rgba = (r << 24) | (g << 16) | (b << 8) | a;

      size_t draw_y = start_y + (height - 1 - py);
      put_pixel(start_x + px, draw_y, rgba);
    }
  }
  term_ypos = height;
  kfree(file_buf);
  kfree(data);
}
void draw_ascii(void) {
  term_color = 0xFFFFFFFF;
  draw_string_term(lykos_ascii);
  draw_string_term("\nWelcome to LykOS\n\n");
  term_color = GREEN;
}

void terminal_init(void) {

  serial_writeln("Starting terminal..");
  set_draw_scale(DEFAULT_TERMINAL_SCALE);

  term_color = 0xFFFFFFFF;
  // draw_string_term(lykos_ascii);
  draw_logo();
  // terminal_newline();

  draw_string_term("\nWelcome to ");
  term_color = MARS_RED;
  terminal_fstring("LykOS\n\n");
  term_color = CYAN;
  draw_string_term("Type help to see a list of commands\n");
  term_color = RED;
  draw_string_term("StarShell>");

  term_color = TEAL;
  draw_cursor_term();

  hist.arena = arena_init(8000);
  hist.start = hist.arena.beg;
  hist.count = 0;

  shell_init();
}

void history_add(const kstring *command) {
  if (hist.count == 100) {
    // reset back to start
    hist.arena.beg = hist.start;
  }

  kstring *dst = &hist.entries[hist.count];
  dst->buf = new(&hist.arena, char, command->len);
  memcpy(dst->buf, command->buf, command->len);
  dst->len = command->len;
  dst->cap = command->len + 1;
  dst->error = false;

  hist.count++;
}

void terminal_newline(void) {
  term_xpos = 0;

  memset(command_content.buf, 0, command_content.len);
  command_content.len = 0;

  u32 temp_color = term_color;
  term_color = RED;
  draw_string_term("StarShell>");
  term_color = temp_color;
}

static void print_history(void) {
  terminal_fstring("\n");
  for (u32 i = 0; i < hist.count; i++) {
    kstring *command = &hist.entries[i];
    terminal_fstring("{kstr}\n", command);
  }
}

bool is_alphanum(char c) {
  if (c >= ' ' && c <= 'z')
    return true;
  else
    return false;
}

#define BACKSPACE '\b'
#define ENTER '\n'

void terminal_process(void) {
  while (1) {
    if (current_input_target == KERNEL_TERMINAL) {
      while (key_event_tail != key_event_head) {
        KeyEvent key = key_event_buffer[key_event_tail];
        key_event_tail = (key_event_tail + 1) % KEYBOARD_BUFFER_SIZE;
        if (key.modifiers & MOD_RELEASE)
          continue;
        terminal_process_input(key);
      }
    }
    preempt_check();
  }
}

void terminal_process_input(KeyEvent input) {
  if (input.key == 'c' && (input.modifiers & MOD_CTRL)) {
    u32 temp_color = term_color;
    term_color = 0xFFFFFFFF;
    draw_string_term("^C");
    term_color = temp_color;
    term_ypos += 8 * g_scale;
    terminal_newline();
    return;
  }

  if (input.key == 'h' && (input.modifiers & MOD_CTRL)) {
    print_history();
    return;
  }

  if (input.key == 'l' && (input.modifiers & MOD_CTRL)) {
    terminal_clearscreen();
    terminal_newline();
    return;
  }

  if (input.key == KEY_DOWN_ARROW) {
    if (hist.count == 0)
      return;
    if (hist_position <= 0)
      return;

    hist_position--;

    serial_fstring("ARROW DOWN: hist_pos={uint}, count={uint}\n", hist_position,
                   hist.count);

    u32 idx = hist.count - hist_position;
    kstring *command = &hist.entries[idx];

    clear_current_command();
    fill_char(term_xpos, term_ypos, BLACK);
    memset(command_content.buf, 0, command_content.len);
    for (u16 i = 0; i < command->len; i++) {
      append_char(&command_content, command->buf[i]);
      draw_char_term(command->buf[i]);
    }
    draw_cursor_term();
    return;
  } else if (input.key == KEY_UP_ARROW) {
    if (hist.count == 0)
      return;
    if (hist_position == hist.count)
      return;

    hist_position++;

    serial_fstring("ARROW UP: hist_pos={uint}, count={uint}\n", hist_position,
                   hist.count);

    u32 idx = hist.count - hist_position;
    kstring *command = &hist.entries[idx];

    clear_current_command();
    fill_char(term_xpos, term_ypos, BLACK);
    memset(command_content.buf, 0, command_content.len);
    for (u16 i = 0; i < command->len; i++) {
      append_char(&command_content, command->buf[i]);
      draw_char_term(command->buf[i]);
    }
    draw_cursor_term();
    return;
  } else if (input.key == ENTER) {
    fill_char(term_xpos + 8 * g_scale, term_ypos, BLACK);
    fill_char(term_xpos, term_ypos, BLACK);
    shell_execute(&command_content);
    draw_cursor_term();
    hist_position = 0;
    return;
  } else if (input.key == BACKSPACE) {
    bool line_not_empty = command_content.len > 0;
    if (line_not_empty) {
      command_content.buf[command_content.len - 1] = '\0';
      command_content.len -= 1;
      fill_char(term_xpos, term_ypos, BLACK);
      term_xpos -= 8 * g_scale;
      fill_char(term_xpos, term_ypos, BLACK);
      draw_cursor_term();

      bool go_back_line = term_xpos == 0;
      if (go_back_line) {
        term_xpos = get_framebuffer()->width;
        term_ypos -= 8 * g_scale;
      }
    }
    return;
  } else if (input.key == KEY_LEFT_ARROW) {
    terminal_fstring("LEFT ARROW");
    return;
  } else if (input.key == KEY_RIGHT_ARROW) {
    terminal_fstring("RIGHT ARROW");
    return;
  }

  if (!is_alphanum(input.key))
    return;

  serial_fstring("Writing char: {char} \n", input.key);
  append_char(&command_content, input.key);
  fill_char(term_xpos, term_ypos, BLACK);
  draw_char_term(input.key);
  serial_fstring("Command buffer size : {int}\n", command_content.len);
  draw_cursor_term();
  hist_position = 0;
}

// drawing

void terminal_fstring(const char *format, ...) {
  va_list args;
  va_start(args, format);

  write_fstring(TERMINAL, format, args);
  va_end(args);
}

void terminal_clearscreen(void) {
  clear_screen(get_framebuffer(), BLACK);
  term_ypos = 8 * g_scale;
}

void draw_char_term(char c) {
  if (c == '\0')
    return;
  if (c == '\n') {
    term_ypos += 8 * g_scale;
    term_xpos = 0;
    return;
  }
  draw_char(c, term_xpos, term_ypos, term_color);
  // advance the cursor
  term_xpos += 8 * g_scale;
  // if next position is greater than the width
  // takes into account how big the START of the next character will be
  if (term_xpos > (get_framebuffer()->width) - (7 * g_scale)) {
    term_ypos += 8 * g_scale;
    term_xpos = 0;
  }
}

static void draw_cursor_term(void) {
  // u32 old_color = term_color;
  // term_color = WHITE;
  // draw_char_term('_');
  // term_color = old_color;
  // Instead of using draw_char_term which would advance the cursor
  // automatically Use draw_char which doesnt modify xpos/ypos
  draw_char('_', term_xpos, term_ypos, WHITE);
}

static void draw_kstring_term(kstring *kstr) {
  for (int i = 0; i < kstr->len; i++) {
    if (kstr->buf[i] == '\n') {
      term_xpos = 0;
      term_ypos += 8 * g_scale;
    } else {
      draw_char_term(kstr->buf[i]);
    }
  }
}

void draw_string_term(const char *str) {
  while (*str) {
    if (*str == '\n') {
      term_xpos = 0;
      term_ypos += 8 * g_scale;
    } else {
      draw_char_term(*str);
    }
    str++;
  }
}

static void clear_current_command(void) {
  bool line_not_empty = command_content.len > 0;
  while (line_not_empty) {
    command_content.len -= 1;
    fill_char(term_xpos, term_ypos, BLACK);
    term_xpos -= 8 * g_scale;
    fill_char(term_xpos, term_ypos, BLACK);
    draw_cursor_term();

    bool go_back_line = term_xpos == 0;
    if (go_back_line) {
      term_xpos = get_framebuffer()->width;
      term_ypos -= 8 * g_scale;
    }
    line_not_empty = command_content.len > 0;
  }
  memset(command_content.buf, 0, command_content.len);
}
