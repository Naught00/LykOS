#pragma once

#include "req.h"
#include <stdbool.h>

// A lot of credit to https://osdev.wiki/wiki/Preemptive_Multitasking

#define USER_STACK_TOP  0x00007FFFFFF00000ULL
#define USER_STACK_PAGES  8

typedef struct Registers {
  u64 rax, rbx, rcx, rdx, rsi, rdi, rsp, rbp;
  u64 r8, r9, r10, r11, r12, r13, r14, r15;
  u64 rflags;
  u64 rip;
  u64 cr3;
} Registers;

#define TASK_STATE_LIST                                                        \
  X(TASK_UNUSED)                                                               \
  X(TASK_READY)                                                                \
  X(TASK_RUNNING)                                                              \
  X(TASK_BLOCKED)                                                              \
  X(TASK_SLEEPING)                                                              \
  X(TASK_DEAD)

enum task_state {
#define X(name) name,
  TASK_STATE_LIST
#undef X
};

// Virtual memory area
typedef struct VMA{
    u64 start;
    u64 end;
    struct VMA *next;
}VMA;

typedef struct Task {
  Registers regs;
  enum task_state state;
  u64 slice;
  const char *name;
  void *stack;
  u64 start_tick;  // used to calculate uptime
  u64 wakeup_tick; // used to implement sleep
  // ELF
  void *segments[8];
  u64 segment_count;
  bool is_elf; // now assumes all elfs are usermode
  // user
  u64 user_rip;
  u64 user_rsp;
  u64 user_cr3;
  VMA* vma_list;

} Task;

#define TASK_STACK_SIZE 0x4000
#define TASK_SLICE_DEFAULT 5
#define MAX_TASKS 128

extern Task tasks[MAX_TASKS];
extern int current_task;
extern int task_count;
extern volatile int need_resched;

void tasking_init(void);
void preempt_check(void);
int task_create(const char *name, void (*entry)(void));
int task_create_elf(const char *name, void *file);
void yield(void);
void debug_tasks(void);
const char *task_state_to_str(enum task_state s);
void task_sleep(u64 ms);
void wake_sleeping_tasks(void);
__attribute__((noreturn)) void task_exit(void);
