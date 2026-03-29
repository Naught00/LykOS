#include "kalloc.h"
#include <stdint.h>
#define KB 1024
#define FRAME_SIZE (4 * KB)
#define TO_KB(bytes) ((bytes) / KB)

void term_stub(char *format, ...) {
	return;
}

#define terminal_fstring term_stub
#define serial_fstring term_stub

#define FREELIST_COALESCE_SIZE 100

typedef struct mem_header {
  u64 size;
  struct mem_header *next;
  struct mem_header *prev;
} mem_header;

static mem_header *free_list_head = NULL;
static u64 freelist_count = 0;

static u64 debug_freelist_size = 0;
static u64 debug_total_allocated = 0;

// returns NULL if no suitable locations
// returns amount of unused bytes in *unused_out
static void *freelist_alloc(u64 size, u64 *unused_out) {
  mem_header *current = free_list_head;
  while (current) {
    // first fit approach- TODO: look into other approaches
    if (current->size >= size) {
      void *addr = (void *)current;

      if (current->prev)
        current->prev->next = current->next;
      else
        free_list_head = current->next;

      if (current->next)
        current->next->prev = current->prev;

      serial_fstring("Allocated from freelist requested {uint}, got {uint}\n",
                     size, current->size);
      *unused_out = current->size - size;
      freelist_count--;
      debug_freelist_size -= current->size - size;
      memset(addr, 0xAA, size);
      return addr;
    }
    current = current->next;
  }

  serial_fstring("Failed to allocate from freelist requested {uint}\n", size);
  return NULL;
}

static void add_to_freelist(u64 addr, u64 size) {
  if (size <= sizeof(mem_header)) {
    return;
  }
  mem_header *hdr = (mem_header *)addr;
  // size doesnt subtract mem_header as the metadata is no longer needed when
  // allocating
  hdr->size = size;
  hdr->next = free_list_head;
  hdr->prev = NULL;

  if (free_list_head)
    free_list_head->prev = hdr;

  free_list_head = hdr;
  serial_fstring("Added to freelist block: block_addr={hex}, size={uint}\n",
                 (u64)hdr, hdr->size);
  debug_freelist_size += (hdr->size) + sizeof(mem_header);
  freelist_count++;
}

// TODO: allignment bugs?
// returns NULL if allocation fails
void *kalloc(u64 size) {
  // allocate size + a u64 to keep track of size of the allocation
  u64 total = size + sizeof(u64);

  void *mem = NULL;
  u64 unused = 0;

  if (size > FRAME_SIZE) {
    mem = mmap2(total);
  } else {
    mem = freelist_alloc(total, &unused);
  }

  if (mem == NULL) {
    mem = mmap2(total);
  }

  if (mem == NULL) {
    serial_fstring("Allocating {uint} failed from freelist and pages\n", total);
    return NULL;
  }
  // memset(mem, 0, sizeof(mem_header));
  memset(mem, 0, size);
  u64 unused_addr = (u64)mem + total;

  serial_fstring("Requested={uint}\n", size);
  serial_fstring("Allocating={uint}\n", size + sizeof(u64));
  serial_fstring("Allocation starts at {hex}\n", (u64)mem);
  serial_fstring("Unused={uint}\n", unused);
  serial_fstring("Unused starts at {hex}\n", (u64)unused_addr);

  // add unused memory to the freelist
  add_to_freelist(unused_addr, unused);

  // TODO: coalesce freelist
  if (freelist_count >= FREELIST_COALESCE_SIZE) {
  }

  debug_total_allocated += size;
  // store size at start of mem
  *(u64 *)mem = size;
  return (void *)((u64)mem + sizeof(u64));
}

void kfree(void *ptr) {
  if (!ptr) return;
  // read the size placed by kmalloc before the ptr
  u64 size = ((u64 *)ptr)[-1];
  u64 *size_addr = (u64 *)(ptr - sizeof(u64));
  serial_fstring("KFree called with {uint}, size of allocation {uint}\n", ptr,
                 size);

  add_to_freelist((u64)size_addr, size + sizeof(u64));
  debug_total_allocated -= size;
}

void *krealloc(void *ptr, size_t size) {
  if (ptr == NULL) {
    return kalloc(size);
  }
  // allocate new block of size size
  // memcpy ptr with ptrsize to size

  u64 ptr_size = ((u64 *)ptr)[-1];
  serial_fstring("Resizing: ptrsize={uint}\n", ptr_size);
  void *new_alloc = kalloc(size);
  if (new_alloc == NULL) {
    serial_fstring("Resizing: failed to allocate memory\n");
    return NULL;
  }
  memcpy(new_alloc, ptr, ptr_size);
  serial_fstring("Resizing: copying to new allocation\n");
  return new_alloc;
}

// debug
bool freelist_has_cycle(mem_header *head) {
  mem_header *slow = head;
  mem_header *fast = head;

  while (fast && fast->next) {
    slow = slow->next;
    fast = fast->next->next;

    if (slow == fast) {
      return true;
    }
  }
  return false;
}

void debug_freelist(void) {
  serial_fstring("freelist_size={uint}\n", debug_freelist_size);
  int index = 0;
  mem_header *current = free_list_head;

  if (freelist_has_cycle(free_list_head)) {
    serial_fstring("Cycle detected in freelist\n");
    return;
  }

  while (current) {
    serial_fstring("IDX={uint} SIZE={uint} ADDR={hex}\n", index,
                   (current->size) + sizeof(mem_header), current);
    terminal_fstring("IDX={uint} SIZE={uint} ADDR={hex}\n", index,
                     (current->size) + sizeof(mem_header), current);
    current = current->next;
    index++;
  }

  terminal_fstring("Items in freelist {uint}, size of freelist KB :{uint}\n",
                   freelist_count, TO_KB(debug_freelist_size));
  serial_fstring("Items in freelist {uint}, size of freelist KB :{uint}\n",
                 freelist_count, TO_KB(debug_freelist_size));
}

void print_allocation_stats(void) {
  terminal_fstring("Total memory allocated (KB): {uint}\n",
                   TO_KB(debug_total_allocated));

  serial_fstring("Total memory allocated (KB): {uint}\n",
                 TO_KB(debug_total_allocated));
}
