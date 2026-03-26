#pragma once 
#include "req.h"
#include <stdbool.h>

// returns region id
i32 shm_create(u64 size, bool public);

// returns addr
u64 shm_map(i32 region_id, u64* out_size);

i64 shm_grant(i32 region_id, u64 target_pid);

i64 shm_unmap(i32 region_id);

i64 shm_destroy(i32 region_id);

#define MAX_SHM_REGIONS  32
#define MAX_SHM_GRANTS   32

typedef struct {
    bool is_active;
    u64  owner_pid;
    u64  phys_addr;
    u32  size;
    u32  num_pages;
    u64  granted_pids[MAX_SHM_GRANTS];
    u32  num_granted;
    bool is_public;
} ShmRegion;

extern ShmRegion shm_regions[MAX_SHM_REGIONS];
