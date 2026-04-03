#include "ps2.h"
#include "arch/x86_64/io.h"
#include "drivers/serial.h"
#include "mem/mem.h"
#include "proc/task.h"
#include "terminal.h"
#include <graphics/draw.h>
#include <stdbool.h>
#include <stdint.h>

#define SHIFT_DOWN 0x2A
#define SHIFT_UP 0xAA
#define CTRL_DOWN 0x1D
#define CTRL_UP 0x9D

volatile u8 keyboard_buffer[KEYBOARD_BUFFER_SIZE];
volatile u8 keyboard_head = 0;
volatile u8 keyboard_tail = 0;

volatile bool shift = false;
volatile bool ctrl = false;
volatile bool arrow_up = false;
volatile bool arrow_down = false;
volatile bool arrow_left = false;
volatile bool arrow_right = false;

volatile u8 key_event_head = 0;
volatile u8 key_event_tail = 0;
volatile KeyEvent *key_event_buffer;

void isr_ps2_keyboard(interrupt_frame *frame) {
  (void)frame;
  char scancode = inb(PS2_DATA_PORT);
  u8 next_head = (keyboard_head + 1) % KEYBOARD_BUFFER_SIZE;
  if (next_head != keyboard_tail) {
    keyboard_buffer[keyboard_head] = scancode;
    keyboard_head = next_head;
  }

  end_of_interrupt();
}

#define EXTENDED 0xE0
u64 key_event_id = 0;

void keyboard_process(void) {
  u64 buf_size = KEYBOARD_BUFFER_SIZE * sizeof(KeyEvent);
  u64 pages = ALIGN_UP(buf_size, PAGE_SIZE) / PAGE_SIZE;
  key_event_buffer = (volatile KeyEvent *)alloc_frames(pages);
  memset((void *)key_event_buffer, 0, buf_size);

  static bool extended = false;
  while (1) {
    while (keyboard_tail != keyboard_head) {
      u8 mods = 0;
      u64 sc = keyboard_buffer[keyboard_tail];
      keyboard_tail = (keyboard_tail + 1) % KEYBOARD_BUFFER_SIZE;
      serial_fstring("sc {uint}\n", sc);

      if (sc == EXTENDED) {
        extended = true;
        continue;
      }

      bool is_release = sc & 0x80;
      sc &= 0x7F;
      if (is_release)
        mods |= MOD_RELEASE;

      switch (sc) {
      case 0x2A:
        shift = !is_release;
        continue;
      case 0x36:
        shift = !is_release;
        continue;
      case 0x1D:
        ctrl = !is_release;
        continue;
      }

      if (shift)
        mods |= MOD_SHIFT;
      if (ctrl)
        mods |= MOD_CTRL;

      u16 key = 0;
      if (extended) {
        extended = false;

        switch (sc) {
        case 0x5B:
          key = KEY_LEFT_META;
          break;
        case 0x48:
          key = KEY_UP_ARROW;
          break;
        case 0x50:
          key = KEY_DOWN_ARROW;
          break;
        case 0x4B:
          key = KEY_LEFT_ARROW;
          break;
        case 0x4D:
          key = KEY_RIGHT_ARROW;
          break;
        default:
          continue;
        }
      } else {
        key = shift ? scancode_to_ascii_caps[sc] : scancode_to_ascii[sc];

        if (key == 0) {
          switch (sc) {
          // case 0x01:
          //   key = KEY_ESCAPE;
          //   break;
          case 0x0E:
            key = KEY_BACKSPACE;
            break;
          case 0x0F:
            key = KEY_TAB;
            break;
          case 0x1C:
            key = KEY_ENTER;
            break;
          case 0x39:
            key = ' ';
            break;
          default:
            continue;
          }
        }
      }

      u64 next = (key_event_head + 1) % KEYBOARD_BUFFER_SIZE;
      if (next == key_event_tail) {
        key_event_tail = (key_event_tail + 1) % KEYBOARD_BUFFER_SIZE;
      }
      key_event_buffer[key_event_head] = (KeyEvent){key_event_id, key, mods};
      key_event_id++;
      key_event_head = next;
    }
    preempt_check();
  }
}
