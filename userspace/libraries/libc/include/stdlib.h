#pragma once
#include <stddef.h>
#include <stdint.h>

void free(void *ptr);
void *malloc(size_t size);
void *realloc(void *ptr, size_t size);
void *calloc(size_t, size_t);

int abs(int);
long strtol(char *nptr, char **endptr, int base);
