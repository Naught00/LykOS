#pragma once
#include <stddef.h>
#include <stdint.h>

void free(void* ptr);
void *malloc(size_t size);
void *realloc(void *ptr, size_t size);
void print_allocation_stats(void);
