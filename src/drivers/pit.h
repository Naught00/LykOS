#include "req.h"
#include <stdbool.h>

#define PIT_HZ 1193182u
#define PIT_CMD 0x43
#define PIT_CH0_DATA 0x40
#define PIT_FREQ_HZ 1000

void pit_init(u32 freq_hz);
void pit_tick(void);
u64 pit_get_ticks(void);
