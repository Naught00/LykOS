#pragma once

#include "req.h"

typedef struct{
  u64 event_id;
  u16 key;
  u8 modifiers;
}KeyEvent;

typedef enum { KERNEL_TERMINAL, USERSPACE } InputTarget;
extern InputTarget current_input_target;

