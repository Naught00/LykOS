#include "mem.h"
#include "drivers/serial.h"
#include "mem/kalloc.h"
#include "req.h"
#include "terminal.h"
#include "vendor/limine.h"
#include <stdbool.h>
#include <stdint.h>

__attribute__((
    used,
    section(".limine_requests"))) static volatile struct limine_memmap_request
    limine_memmap_request = {.id = LIMINE_MEMMAP_REQUEST, .revision = 0};

__attribute__((
    used,
    section(
        ".limine_requests"))) static volatile struct limine_paging_mode_request
    limine_paging_mode_request = {.id = LIMINE_PAGING_MODE_REQUEST,
                                  .revision = 3};
__attribute__((
    used,
    section(".limine_requests"))) static volatile struct limine_hhdm_request
    limine_hhdm_request = {.id = LIMINE_HHDM_REQUEST, .revision = 0};

static u64 total_usable = 0;
static u64 reclaimable = 0;
static u64 bad = 0;
static u64 reserved = 0;
static u64 framebuf = 0;
static u64 nvs = 0;
static u64 exe = 0;
static u64 highest_addr = 0;

u64 hhdm_offset;
u64 *kernel_pml4;

void memmap_init(void) {
  struct limine_memmap_response *response = limine_memmap_request.response;
  if (!response) {
    serial_writeln("No memory map from Limine!");
    return;
  }

  hhdm_offset = limine_hhdm_request.response->offset;
  serial_fstring("hhdm offset {uint}\n", hhdm_offset);

  // store Limines PML4
  u64 cr3;
  asm volatile("mov %%cr3, %0" : "=r"(cr3));
  kernel_pml4 = (u64 *)phys_to_virt(cr3 & PTE_ADDR_MASK);

  serial_fstring("Memory map entries: {int}\n", response->entry_count);

  for (u64 i = 0; i < response->entry_count; i++) {
    struct limine_memmap_entry *entry = response->entries[i];
    if (entry->base > highest_addr)
      highest_addr = entry->base;

    char *is_usable = "True";
    if (entry->type != LIMINE_MEMMAP_USABLE)
      is_usable = "False";

    serial_fstring("Region {uint}: base={uint} length={uint} usable={str}\n", i,
                   entry->base, entry->length, is_usable);

    terminal_fstring("Region {uint}: base={uint} size={uint} type=", i,
                     entry->base, entry->length, entry->type);

    switch (entry->type) {
    case (LIMINE_MEMMAP_USABLE):
      terminal_fstring("USABLE\n");
      total_usable += entry->length;
      break;
    case (LIMINE_MEMMAP_RESERVED):
      terminal_fstring("RESERVED\n");
      reserved += entry->length;
      break;
    case (LIMINE_MEMMAP_BAD_MEMORY):
      terminal_fstring("BAD MEMORY\n");
      bad += entry->length;
      break;
    case (LIMINE_MEMMAP_FRAMEBUFFER):
      terminal_fstring("FRAMEBUFFER\n");
      framebuf += entry->length;
      break;
    case (LIMINE_MEMMAP_ACPI_NVS):
      terminal_fstring("ACPI_NVS\n");
      nvs += entry->length;
      break;
    case (LIMINE_MEMMAP_EXECUTABLE_AND_MODULES):
      terminal_fstring("EXECUTABLE MODULES\n");
      exe += entry->length;
      break;
    case (LIMINE_MEMMAP_BOOTLOADER_RECLAIMABLE):
      terminal_fstring("BOOTLOADER RECLAIMABLE\n");
      reclaimable += entry->length;
      break;
    case (LIMINE_MEMMAP_ACPI_RECLAIMABLE):
      terminal_fstring("ACPI RECLAIMABLE\n");
      reclaimable += entry->length;
      break;
    }
  }
}

// bitmap will have 1 frame per bit
static u8 frame_bitmap[MAX_FRAMES / 8];

static inline void set_bit(u64 frame) {
  frame_bitmap[frame / 8] |= (1 << (frame % 8));
}

static inline void clear_bit(u64 frame) {
  frame_bitmap[frame / 8] &= ~(1 << (frame % 8));
}

static inline int test_bit(u64 frame) {
  return frame_bitmap[frame / 8] & (1 << (frame % 8));
}

void mark_frames_free(u64 start, u64 count) {
  for (u64 i = 0; i < count; i++) {
    clear_bit(start + i); // 0 = free
  }
}

void mark_frames_used(u64 start, u64 count) {
  for (u64 i = 0; i < count; i++) {
    set_bit(start + i); // 1 = used
  }
}

void pmm_init(void) {
  memset(frame_bitmap, 0xFF, sizeof(frame_bitmap));
  struct limine_memmap_response *response = limine_memmap_request.response;
  for (u64 i = 0; i < response->entry_count; i++) {
    struct limine_memmap_entry *entry = response->entries[i];
    if (entry->type == LIMINE_MEMMAP_USABLE) {
      u64 base = entry->base;
      u64 len = entry->length;

      // skip anything below 1MB
      // HACK: uefi bug
      // if (base < MB) {
      //   if (base + len <= MB)
      //     continue;
      //   len -= (MB - base);
      //   base = MB;
      // }

      serial_fstring("Entry is usable at base {uint} \n", entry->base);
      u64 start = base / FRAME_SIZE;
      u64 count = len / FRAME_SIZE;
      serial_fstring("Marking {uint} frames free at {uint}\n", count, start);
      mark_frames_free(start, count);
    }
  }
}

int64_t find_first_free_frame(void) {
  for (int64_t i = 1; i < MAX_FRAMES / 8; i++) { // avoid first frame
    if (frame_bitmap[i] != 0xFF) {               // has to have one non zero bit
      for (int b = 0; b < 8; b++) {
        if (!(frame_bitmap[i] & (1 << b))) {
          return i * 8 + b;
        }
      }
    }
  }
  return -1; // none
}

// can null
void *alloc_frames(u64 frame_amount) {
  u64 avail_contigous_frames = 0;
  u64 start_idx = 0;
  for (int64_t i = 1; i < MAX_FRAMES / 8; i++) { // avoid first frame
    // TODO:possible optimisation here using amount sizes
    if (frame_bitmap[i] != 0xFF) { // has to have one non zero bit
      for (int b = 0; b < 8; b++) {
        if (!(frame_bitmap[i] & (1 << b))) {
          // we have found a free frame
          if (start_idx == 0)
            start_idx = i * 8 + b;
          avail_contigous_frames++;

          if (avail_contigous_frames == frame_amount) {
            mark_frames_used(start_idx, frame_amount);
            serial_fstring("Allocating idx:{uint}, frames {uint}\n", start_idx,
                           avail_contigous_frames);
            serial_fstring("Allocation at 0x{hex}, phys 0x{hex}\n",
                           (start_idx * FRAME_SIZE) +
                               limine_hhdm_request.response->offset,
                           start_idx * FRAME_SIZE);
            return (void *)((start_idx * FRAME_SIZE) +
                            limine_hhdm_request.response->offset);
          }
        } else { // frame isnt free, reset counters
          avail_contigous_frames = 0;
          start_idx = 0;
        }
      }
    }
  }
  return NULL;
}

// can null
void *pmm_alloc_frame(void) {
  int64_t idx = find_first_free_frame();
  serial_fstring("Allocating frame {uint}\n", idx);
  serial_fstring("Frame base: {uint}\n Frame virt: {uint}\n", idx * FRAME_SIZE,
                 (idx * FRAME_SIZE) + limine_hhdm_request.response->offset);

  if (idx == -1)
    return NULL;
  set_bit(idx);
  return (void *)((idx * FRAME_SIZE) + limine_hhdm_request.response->offset);
}

void pmm_free_frame(void) { return; }

//
// debug
//
void print_memory_stats(void) {
  u64 free_frames = 0;
  for (int64_t i = 1; i < MAX_FRAMES / 8; i++) { // avoid first frame
    if (frame_bitmap[i] != 0xFF) {
      for (int b = 0; b < 8; b++) {
        if (!(frame_bitmap[i] & (1 << b))) {
          free_frames += 1;
        }
      }
    }
  }
  u64 free_frames_MB = TO_MB(free_frames * FRAME_SIZE);
  u64 used_frames_MB = TO_MB(total_usable - (free_frames * FRAME_SIZE));
  terminal_fstring("Free memory (MB): {uint}\n", free_frames_MB);
  terminal_fstring("Used memory (MB): {uint}\n", used_frames_MB);
  print_allocation_stats();
}

void print_paging(void) {
  struct limine_paging_mode_response response =
      *limine_paging_mode_request.response;

  terminal_fstring("Kernel offset {uint}\n",
                   limine_hhdm_request.response->offset);
  terminal_fstring("Kernel offset {hex}", limine_hhdm_request.response->offset);
  terminal_fstring("\nPAGING\n");
  terminal_fstring("Mode : {uint}\n", response.mode);
  terminal_fstring("Revision: {uint}\n", response.revision);
  terminal_fstring("Max Mode: {uint}\n", limine_paging_mode_request.max_mode);
  terminal_fstring("Min Mode: {uint}\n", limine_paging_mode_request.min_mode);
  terminal_fstring("Mode: {uint}\n", limine_paging_mode_request.mode);
}

void print_memmap(void) {
  terminal_fstring("\n Total usable~      (MB):{uint}\n", TO_MB(total_usable));
  terminal_fstring("\n Total reclaimable~ (KB):{uint}\n", TO_KB(reclaimable));
  terminal_fstring("\n Total bad~         (KB):{uint}\n", TO_KB(bad));
  terminal_fstring("\n Total reserved~    (MB):{uint}\n", TO_MB(reserved));
  terminal_fstring("\n Total exe~         (KB):{uint}\n", TO_KB(exe));
  terminal_fstring("\n Total framebuffer~ (MB):{uint}\n", TO_MB(framebuf));
  terminal_fstring("\n Total nvs?~        (KB):{uint}\n", TO_KB(nvs));
  terminal_fstring("\n Bitmap size        (MB):{uint}\n",
                   TO_MB(highest_addr / (8 * FRAME_SIZE)));
}

// Maps a single 4KB page virt_addr to phys_addr
// in the given page table. Allocates intermediate tables if needed.
void map_page(u64 *pml4, u64 virt_addr, u64 phys_addr, u64 flags) {
  serial_fstring("Mapping {uint} to {uint}\n", virt_addr, phys_addr);
  // Extract the 9-bit index at each level from the virtual address
  u64 pml4_idx = (virt_addr >> 39) & 0x1FF;
  u64 pdpt_idx = (virt_addr >> 30) & 0x1FF;
  u64 pd_idx = (virt_addr >> 21) & 0x1FF;
  u64 pt_idx = (virt_addr >> 12) & 0x1FF;

  // Walk PML4 -> PDPT
  // If no PDPT exists yet, allocate one
  if (!(pml4[pml4_idx] & PTE_PRESENT)) {
    void *new_table = pmm_alloc_frame(); // returns virtual addr
    memset(new_table, 0, FRAME_SIZE);
    pml4[pml4_idx] =
        virt_to_phys(new_table) | PTE_PRESENT | PTE_WRITE | PTE_USER;
  }
  u64 *pdpt = (u64 *)phys_to_virt(pml4[pml4_idx] & PTE_ADDR_MASK);

  // Walk PDPT -> PD
  if (!(pdpt[pdpt_idx] & PTE_PRESENT)) {
    void *new_table = pmm_alloc_frame();
    memset(new_table, 0, FRAME_SIZE);
    pdpt[pdpt_idx] =
        virt_to_phys(new_table) | PTE_PRESENT | PTE_WRITE | PTE_USER;
  }
  u64 *pd = (u64 *)phys_to_virt(pdpt[pdpt_idx] & PTE_ADDR_MASK);

  // Walk PD -> PT
  if (!(pd[pd_idx] & PTE_PRESENT)) {
    void *new_table = pmm_alloc_frame();
    memset(new_table, 0, FRAME_SIZE);
    pd[pd_idx] = virt_to_phys(new_table) | PTE_PRESENT | PTE_WRITE | PTE_USER;
  }
  u64 *pt = (u64 *)phys_to_virt(pd[pd_idx] & PTE_ADDR_MASK);

  // Set the actual page mapping
  pt[pt_idx] = phys_addr | flags;
}

// Creates a new address space with kernel mappings in the upper half.
// Returns the virtual address of the new PML4, or NULL on failure.
u64 *create_address_space(void) {
  u64 *new_pml4 = (u64 *)pmm_alloc_frame();
  if (!new_pml4)
    return NULL;

  memset(new_pml4, 0, FRAME_SIZE);
  // Copy the upper 256 entries (kernel half) from Limine's PML4.
  // PML4 has 512 entries. Entries 0-255 cover the lower half (user space).
  // Entries 256-511 cover the upper half (kernel space).
  // By copying these, the kernel is accessible in every address space.
  for (int i = 256; i < 512; i++) {
    new_pml4[i] = kernel_pml4[i];
  }

  return new_pml4;
}

// Switch to a different address space by loading its PML4 into CR3
void switch_address_space(u64 *pml4) {
  u64 cr3;
  asm volatile("mov %%cr3, %0" : "=r"(cr3));
  serial_fstring("Switching addr space from 0x{hex} to 0x{hex}\n", cr3, pml4);
  u64 phys = virt_to_phys((void *)pml4);
  asm volatile("mov %0, %%cr3" : : "r"(phys) : "memory");
}

void debug_cr3(void) {
  u64 cr3;
  asm volatile("mov %%cr3, %0" : "=r"(cr3));
  // assuming if addr space is not kernel its userspace
  // although we could have ring 0 processes
  if (&cr3 == kernel_pml4) {
    serial_fstring("KERNEL SPACE : TRUE\n");
  } else {
    serial_fstring("USER SPACE : TRUE\n");
  }
  serial_fstring("CR3 : 0x{hex}\n", cr3);
}

// adds virtual memory area of start -> end in Task t
void add_vma(Task *t, u64 start, u64 end) {
  VMA *v = kalloc(sizeof(VMA));
  v->start = start;
  v->end = end;

  // Insert sorted by start address
  VMA **pp = &t->vma_list;
  while (*pp && (*pp)->start < start)
    pp = &(*pp)->next;

  v->next = *pp;
  *pp = v;
}

void print_vma(VMA *vma) {
  terminal_fstring("[0x{hex} - 0x{hex}] ", vma->start, vma->end);
  terminal_fstring("Size: (KB){uint} | Pages: {uint}\n",
                   TO_KB(vma->end - vma->start),
                   (vma->end - vma->start) / FRAME_SIZE);

  serial_fstring("[0x{hex} - 0x{hex}]\n", vma->start, vma->end);
}
void debug_vmas(Task *t) {
  serial_fstring("VMAs for task '{str}':\n", t->name);
  terminal_fstring("VMAs for task '{str}':\n", t->name);
  VMA *v = t->vma_list;
  while (v) {
    print_vma(v);
    v = v->next;
  }
}

#define MMAP_REGION_START 0x10000000
#define MMAP_REGION_END 0x1910000000

u64 find_free_region(Task *t, u64 size) {
  size = ALIGN_UP(size, PAGE_SIZE);

  u64 candidate = MMAP_REGION_START;
  VMA *next = t->vma_list;

  while (next) {
    if (candidate + size < next->start) {
      return candidate;
    }
    if (next->end > candidate)
      candidate = next->end;
    next = next->next;
  }

  if (candidate + size <= MMAP_REGION_END)
    return candidate;

  serial_fstring(
      "No space in virtual address space for MMAP task {uint} '{str}'\n",
      current_task, t->name);
  return 0;
}
