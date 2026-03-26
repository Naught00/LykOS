#include "lykosapi.h"

void main(void) {
  u64 *mem = (u64 *)mmap(4097);
  u64 count = 8192 / 8;

  for (u64 i = 0; i < count; i++)
    mem[i] = i;

  if (mem[0] == 0 && mem[count - 1] == count - 1)
    write("2 pages used for mmap\n");
}
