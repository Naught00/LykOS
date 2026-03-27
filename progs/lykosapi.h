#include "../src/req.h"
#include <stdbool.h>

typedef struct{
  u16 key;
  u8 modifiers;
} KeyEvent;


#define MAILBOX_MESSAGE_SIZE 128
typedef struct {
  u64 sender_pid;
  u64 len;
  char data[MAILBOX_MESSAGE_SIZE];
} MailboxMessage;

#define SYS_EXIT 0
#define SYS_SLEEP 1
#define SYS_WRITE 2
#define SYS_MMAP_FB 3
#define SYS_GET_KV 4
#define SYS_MMAP 5
#define SYS_EXEC 6
#define SYS_MAILBOX_CREATE 7
#define SYS_MAILBOX_SEND 8
#define SYS_MAILBOX_RECEIVE 9
#define SYS_SHM_CREATE 10
#define SYS_SHM_MAP 11

// a = rax, D = rdi, S = rsi, d = rdx
// syscall with 0 args
static inline u64 syscall0(u64 num) {
    int64_t ret;
    asm volatile("int $0x80"
        : "=a"(ret)
        : "a"(num)
        : "rcx", "r11", "memory");
    return ret;
}

// 1 arg
static inline i64 syscall1(u64 num, u64 arg1) {
    int64_t ret;
    asm volatile("int $0x80"
        : "=a"(ret)
        : "a"(num), "D"(arg1)
        : "rcx", "r11", "memory");
    return ret;
}

// 2 args
static inline i64 syscall2(u64 num, u64 arg1, u64 arg2) {
    int64_t ret;
    asm volatile("int $0x80"
        : "=a"(ret)
        : "a"(num), "D"(arg1), "S"(arg2)
        : "rcx", "r11", "memory");
    return ret;
}

// 3 args
static inline i64 syscall3(u64 num, u64 arg1, u64 arg2,
                               u64 arg3) {
    int64_t ret;
    asm volatile("int $0x80"
        : "=a"(ret)
        : "a"(num), "D"(arg1), "S"(arg2), "d"(arg3)
        : "rcx", "r11", "memory");
    return ret;
}
static inline i64 lykos_exit(void) { return syscall0(SYS_EXIT); }

static inline i64 sleep(u64 ms) { return syscall1(SYS_SLEEP, ms); }

static inline i64 write(char *str) { return syscall1(SYS_WRITE, (u64)str); }

static inline u32 *mmap_fb(void) { return (u32 *)syscall0(SYS_MMAP_FB); }

static inline i64 get_key_event(KeyEvent *ev_ptr) {
  return syscall1(SYS_GET_KV, (u64)ev_ptr);
}

static inline u64 mmap(u64 size) { return syscall1(SYS_MMAP, size);}

static inline i64 exec(char *file_name) { return syscall1(SYS_EXEC, (u64)file_name);}

static inline i64 mbox_create(u64 id) { return syscall1(SYS_MAILBOX_CREATE, id); }

static inline i64 mbox_send(u64 id, char* data, u64 data_len) { return syscall3(SYS_MAILBOX_SEND, id, (u64)data, data_len); }

static inline i64 mbox_receive(u64 id, MailboxMessage *out) { return syscall2(SYS_MAILBOX_RECEIVE, id, (u64)out); }

static inline i32 shm_create(u64 size, bool public) { return syscall2(SYS_SHM_CREATE, size, public); }

static inline u64 shm_map(i32 region_id, u64* out_size) { return syscall2(SYS_SHM_MAP, region_id, (u64)out_size); }

