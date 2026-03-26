#include "shm.h"
#include "mem/kalloc.h"
#include "mem/mem.h"
#include "proc/task.h"

ShmRegion shm_regions[MAX_SHM_REGIONS];

i32 shm_create(u64 size, bool public) {
  for (int i = 0; i < MAX_SHM_REGIONS; i++) {
    if (!shm_regions[i].is_active) {
      ShmRegion *shm = &shm_regions[i];
      u64 phys_addr = virt_to_phys(kalloc(size));
      *shm = (ShmRegion){
          .is_active = true,
          .owner_pid = current_task,
          .phys_addr = phys_addr,
          .size = size,
          .num_granted = 0,
          .is_public = public,
      };
      return i;
    }
  }

  return -1;
}

u64 shm_map(i32 region_id, u64 *out_size) {
  ShmRegion shm = shm_regions[region_id];
  if (!shm.is_active)
    return 0;

  Task *t = &tasks[current_task];
  u64 addr = find_free_region(t, shm.size);

  u64 pages = ALIGN_UP(shm.size, PAGE_SIZE) / PAGE_SIZE;
  for (u64 i = 0; i < pages; i++) {
    u64 virt = addr + (i * FRAME_SIZE);

    map_page(phys_to_virt(t->user_cr3), virt, shm.phys_addr + (i * FRAME_SIZE),
             PTE_PRESENT | PTE_USER | PTE_WRITE);
  }

  add_vma(t, addr, addr + (pages * FRAME_SIZE));
  *out_size = shm.size;
  return addr;
}
