#pragma once
#include <stddef.h>
#include <stdint.h>

void kfree(void* ptr);
void *kalloc(u64 size);
void *krealloc(void *ptr, size_t size);
void debug_freelist(void);
void print_allocation_stats(void);
