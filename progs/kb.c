#include "lykosapi.h"

#define MOD_SHIFT (1 << 0)
#define MOD_CTRL (1 << 1)
#define MOD_ALT (1 << 2)
#define MOD_RELEASE (1 << 3)

void main(void) {

  u64 kb_size;
  volatile KeyEvent *kb = (KeyEvent *)mmap_keyboard(&kb_size);

  u64 idx = 0;
  u64 last_id = 0;
  while (1) {
    u32 next = (idx + 1) % kb_size;
    if (kb[next].event_id > last_id) {
      last_id = kb[next].event_id;
      idx = next;
      if (kb[idx].key == 'a')
        write("Pressed a");
      if (kb[idx].key == 'a' && kb[idx].modifiers & MOD_RELEASE)
        write("Released a");
      if (kb[idx].key == 'b')
        write("Pressed b");
    }
  }
}
