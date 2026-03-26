#include "vfs.h"
#include "drivers/serial.h"
#include "fs/fat16.h"
#include "graphics/draw.h"
#include "klib/kstring.h"
#include "mem/kalloc.h"
#include "terminal.h"
#include <stdbool.h>
#define STB_DS_IMPLEMENTATION
#define STBDS_REALLOC(context, ptr, size) krealloc(ptr, size)
#define STBDS_FREE(context, ptr) kfree(ptr)
#include "vendor/stb_ds.h"
#include <stdint.h>

//  TODO: make this file generic over a filesystem
// currently relies directly on the fat16 functions, while i figure out the API

static void normalise(const uint8_t old[11], char new[13]) {
  int pos = 0;
  for (int i = 0; i < 8; i++) {
    if (old[i] == ' ')
      break;
    new[pos++] = old[i];
  }

  if (old[8] != ' ') {
    new[pos++] = '.';
    for (int i = 8; i < 11; i++) {
      if (old[i] == ' ')
        break;
      new[pos++] = old[i];
    }
  }

  new[pos] = '\0';
}

// TODO: path traversal, not just assume root
void list_files(kstring *path) {
  fat16_dir_entry_t *files = kalloc(100 * sizeof(fat16_dir_entry_t));
  u32 file_count = fat16_get_entries(files);
  if (file_count < 1)
    terminal_fstring("\nNo files found!");
  for (u32 i = 0; i < file_count; i++) {
    char name[13];
    normalise(files[i].name, name);
    if (files[i].attr & 0x10) {

      u32 temp_color = term_color;
      term_color = CRIMSON;
      terminal_fstring("DIR:");
      terminal_fstring("  {str}\n", name);
      term_color = temp_color;
    } else {
      u32 temp_color = term_color;
      term_color = GREEN;
      terminal_fstring("FILE:");

      // if (strncmp("ELF", name + 11, 3))
      // term_color = GREEN;
      term_color = LIME;
      terminal_fstring(" {str}\n", name);
      term_color = temp_color;
    }
  }
  kfree(files);
  return;
}

// returns NULL if file cant be found, or returns ptr to file
// size of file in out_size
void *read_file(kstring *path, u64 *out_size) {
  to_upper(path);

  fat16_dir_entry_t *files = kalloc(100 * sizeof(fat16_dir_entry_t));
  u32 file_count = fat16_get_entries(files);

  if (file_count < 1)
    terminal_fstring("No file {kstr} found\n", path);

  for (u32 i = 0; i < file_count; i++) {
    fat16_dir_entry_t file = files[i];
    bool is_file = !(file.attr & 0x10);
    char file_name[13];
    normalise(file.name, file_name);

    bool file_found =
        is_file && (strncmp(file_name, path->buf, path->len) == 0);
    if (file_found) {
      // terminal_fstring("File found! :{str}\n", file_name);

      *out_size = file.file_size;
      char *file_buf = kalloc(file.file_size);
      fat16_read_file(&file, (uint8_t *)file_buf);
      kfree(files);
      return file_buf;
    }
  }

  kfree(files);
  return NULL;
}

// free ks.buf
// static inline kstring make_kstring(const char *src, size_t len) {
//   kstring ks;
//   ks.buf = kalloc(len + 1);
//   ks.len = len;
//   ks.cap = len + 1;
//   memcpy(ks.buf, src, len);
//   ks.buf[len] = '\0';
//   return ks;
// }

kstring *vfs_split_path(const kstring *path) {
  kstring *parts = NULL;

  u16 i = 1;

  while (i < path->len) {
    u16 start = i;
    while (i < path->len && path->buf[i] != '/')
      i++;

    u16 len = i - start;
    if (len > 0) {
      kstring component = make_kstring(&path->buf[start], len);
      arrput(parts, component);
    }

    // skip subsequent /
    while (i < path->len && path->buf[i] == '/')
      i++;
  }

  return parts;
}

// takes in stb_ds arr of kstring*
// used for swithing directories
// traverse directories from root
bool valid_path(kstring *path) {
  if (path->buf[0] != '/') {
    serial_fstring("VFS: (valid_path) Path must be absolute\n");
    return false;
  }
  kstring *dir_arr = vfs_split_path(path);
  u8 dir_count = arrlen(dir_arr);
  serial_fstring("Directories in path={uint}\n", dir_count);
  terminal_fstring("Directories in path={uint}\n", dir_count);

  // TODO:
  fat16_dir_entry_t *files = kalloc(sizeof(fat16_dir_entry_t) * 512);
  u32 count = fat16_get_entries(files);
  for (int i = 0; i < arrlen(dir_arr); i++) {
    terminal_fstring("idx={uint}, dir={str}\n", i, dir_arr[i].buf);
  }

  arrfree(dir_arr);
  return true;
}
