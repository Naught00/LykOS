#pragma once
#include "drivers/pit.h"

#include "req.h"

void ticks_to_time(u64 ticks, char out[9]);
static inline u64 ms_to_ticks(u64 ms) { return ms * PIT_FREQ_HZ / 1000; }

u64 get_seconds(void);
u64 get_minutes(void);
u64 get_hours(void);
u64 clock_seconds(void);
u64 clock_minutes(void);
