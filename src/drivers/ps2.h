#pragma once
#include "arch/x86_64/idt.h"
#include "user/input.h"
#include <stdbool.h>
#include <stdint.h>

#define ACK 0xFA
#define RESEND 0xFE
#define PS2_DATA_PORT 0x60

#define KEYBOARD_BUFFER_SIZE 128

#define KEY_ESCAPE      27
#define KEY_BACKSPACE   257
#define KEY_TAB         258
#define KEY_ENTER       259

#define KEY_UP_ARROW 301
#define KEY_DOWN_ARROW 302
#define KEY_LEFT_ARROW 303
#define KEY_RIGHT_ARROW 304

#define MOD_SHIFT  (1 << 0)
#define MOD_CTRL   (1 << 1)
#define MOD_ALT    (1 << 2)
#define MOD_RELEASE (1 << 3)

extern volatile bool shift;
extern volatile bool ctrl;
extern volatile bool arrow_up;
extern volatile bool arrow_down;
extern volatile bool arrow_left;
extern volatile bool arrow_right;


// Circular buffer for scancodes
extern volatile u8 keyboard_buffer[KEYBOARD_BUFFER_SIZE];
extern volatile u8 keyboard_head;
extern volatile u8 keyboard_tail;

extern volatile KeyEvent *key_event_buffer;
extern volatile u8 key_event_head;
extern volatile u8 key_event_tail;

void keyboard_process(void);
void keyboard_process(void);
void isr_ps2_keyboard(interrupt_frame *frame);

static char scancode_to_ascii[128] = {
    0,   27,  '1',  '2',  '3',  '4', '5', '6',  '7', '8', '9', '0',
    '-', '=', '\b', '\t', 'q',  'w', 'e', 'r',  't', 'y', 'u', 'i',
    'o', 'p', '[',  ']',  '\n', 0,   'a', 's',  'd', 'f', 'g', 'h',
    'j', 'k', 'l',  ';',  '\'', '`', 0,   '\\', 'z', 'x', 'c', 'v',
    'b', 'n', 'm',  ',',  '.',  '/', 0,   '*',  0,   ' ',
};

static char scancode_to_ascii_caps[128] = {
    0,   27,  '!',  '@',  '#',  '$', '%', '^', '&', '*', '(', ')',
    '_', '+', '\b', '\t', 'Q',  'W', 'E', 'R', 'T', 'Y', 'U', 'I',
    'O', 'P', '{',  '}',  '\n', 0,   'A', 'S', 'D', 'F', 'G', 'H',
    'J', 'K', 'L',  ':',  '"',  '~', 0,   '|', 'Z', 'X', 'C', 'V',
    'B', 'N', 'M',  '<',  '>',  '?', 0,   '*', 0,   ' ',
};
