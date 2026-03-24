#include "../src/req.h"

typedef struct{
  u16 key;
  u8 modifiers;
} KeyEvent;

#define SYS_EXIT 0
#define SYS_SLEEP 1
#define SYS_WRITE 2
#define SYS_MMAP_FB 3
#define SYS_GET_KV 4
#define SYS_MMAP 5

// a = rax, D = rdi, S = rsi, d = rdx
// syscall with 0 args
static inline u64 syscall0(u64 num) {
  int64_t ret;
  asm volatile("int $0x80" : "=a"(ret) : "a"(num));
  return ret;
}

// 1 arg
static inline int64_t syscall1(uint64_t num, uint64_t arg1) {
  int64_t ret;
  asm volatile("int $0x80" : "=a"(ret) : "a"(num), "D"(arg1));
  return ret;
}

// 2 args
static inline int64_t syscall2(uint64_t num, uint64_t arg1, uint64_t arg2) {
  int64_t ret;
  asm volatile("int $0x80" : "=a"(ret) : "a"(num), "D"(arg1), "S"(arg2));
  return ret;
}

// 3 args
static inline int64_t syscall3(uint64_t num, uint64_t arg1, uint64_t arg2,
                               uint64_t arg3) {
  int64_t ret;
  asm volatile("int $0x80"
               : "=a"(ret)
               : "a"(num), "D"(arg1), "S"(arg2), "d"(arg3));
  return ret;
}

static inline i64 lykos_exit(void) { return syscall0(SYS_EXIT); }

static inline i64 sleep(u64 ms) { return syscall1(SYS_SLEEP, ms); }

static inline i64 write(char *str) { return syscall1(SYS_WRITE, (u64)str); }

static inline u32 *mmap_fb(void) { return (u32 *)syscall0(SYS_MMAP_FB); }

static inline i64 get_key_event(KeyEvent *ev_ptr) {
  return syscall1(SYS_GET_KV, (u64)ev_ptr);
}

static inline void *mmap2(u64 size) { return (void *) syscall1(SYS_MMAP, size);}
