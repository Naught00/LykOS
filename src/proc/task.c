#include "task.h"
#include "arch/x86_64/gdt.h"
#include "drivers/pit.h"
#include "drivers/serial.h"
#include "klib/kstring.h"
#include "klib/time.h"
#include "mem/kalloc.h"
#include "mem/mem.h"
#include "terminal.h"
#include "user/elf.h"

Task tasks[MAX_TASKS];
int current_task = -1;
int task_count = 0;
volatile int need_resched = 0;

extern void switch_task(Registers *from, Registers *to); // in switch_task.asm
extern void enter_usermode(u64 rip, u64 rsp, u64 cr3);
__attribute__((noreturn)) void task_exit(void);

static int find_free_task_slot(void) {
  for (int i = 0; i < MAX_TASKS; i++) {
    if (tasks[i].state == TASK_UNUSED || tasks[i].state == TASK_DEAD) {
      return i;
    }
  }
  return -1;
}

int task_create(const char *name, void (*entry)(void)) {
  asm volatile("cli"); // we dont wanna get pre-empted during a yield
  int i = find_free_task_slot();
  if (i < 0) {
    serial_fstring("[TASK] No free task slots\n");
    return -1;
  }

  Task *t = &tasks[i];

  t->stack = kalloc(TASK_STACK_SIZE);
  if (!t->stack) {
    serial_fstring("[TASK] Failed to allocate stack\n");
    return -1;
  }

  u64 stack_top = ((u64)t->stack + TASK_STACK_SIZE) & ~0xFULL;
  // fake return address so if entry() returns, it goes to task_exit
  stack_top -= 8;
  *(u64 *)stack_top = (u64)task_exit;

  t->regs.r15 = 0;
  t->regs.r14 = 0;
  t->regs.r13 = 0;
  t->regs.r12 = 0;
  t->regs.rbx = 0;
  t->regs.rbp = 0;
  t->regs.rip = (u64)entry;
  t->regs.rsp = stack_top;
  t->regs.cr3 = virt_to_phys((void *)kernel_pml4);
  t->regs.rflags = 0x202;

  t->state = TASK_READY;
  t->slice = TASK_SLICE_DEFAULT;
  t->name = name;
  t->start_tick = pit_get_ticks(); // used to calculate uptime
  t->is_elf = false;

  if (i >= task_count)
    task_count = i + 1;

  serial_fstring("[TASK] Created: {str}\n", name);

  asm volatile("sti");
  return i;
}

static int find_next_ready_task(void) {
  if (task_count == 0)
    return -1;

  for (int offset = 1; offset <= task_count; offset++) {
    int i = (current_task + offset) % task_count;
    if (tasks[i].state == TASK_READY) {
      return i;
    }
  }

  return -1;
}

void elf_trampoline(void) {
  Task *t = &tasks[current_task];
  enter_usermode(t->user_rip, t->user_rsp, t->user_cr3);
}

void yield(void) {
  // serial_fstring("Current task {str}\n", tasks[current_task].name);
  // for (int task_id = 0; task_id < MAX_TASKS; task_id++) {
  //   Task *t = &tasks[task_id];
  //   if (t->state == TASK_UNUSED)
  //     continue;
  //   serial_fstring("Task {str} state {str} id {uint}\n", t->name, t->state,
  //   task_id);
  // }

  asm volatile("cli"); // we dont wanna get pre-empted during a yield
  u64 cr3;
  asm volatile("mov %%cr3, %0" : "=r"(cr3));
  serial_fstring("yield: current cr3 = {uint}\n", cr3);
  serial_fstring("yield: kernel cr3 = {uint}\n",
                 virt_to_phys((void *)kernel_pml4));
  serial_fstring("Current task {uint} : {str} yielding\n", current_task,
                 tasks[current_task].name);
  if (current_task < 0) {
    goto end;
  }

  int next_id = find_next_ready_task();
  if (next_id < 0 || next_id == current_task) {
    goto end;
  }

  int prev = current_task;
  Task *next = &tasks[next_id];

  serial_fstring("yield: next cr3 = {uint}\n", next->regs.cr3);
  tss_set_kernel_stack((u64)next->stack + TASK_STACK_SIZE);

  if (tasks[prev].state == TASK_RUNNING)
    tasks[prev].state = TASK_READY;

  next->state = TASK_RUNNING;
  next->slice = TASK_SLICE_DEFAULT;
  current_task = next_id;

  serial_fstring("Switching to {str} \n", tasks[next_id].name);
  serial_fstring("Resuming {str} rip={hex} rsp={hex}\n", next->name,
                 next->regs.rip, next->regs.rsp);
  serial_fstring("  rsp before switch = {hex}\n", tasks[prev].regs.rsp);
  serial_fstring("Saving {str}\n", tasks[prev].name);
  switch_task(&tasks[prev].regs, &next->regs);

end:
  asm volatile("sti");
}

// TODO: need a freetask function so task_kill and exit dont have duplicated
// code
__attribute__((noreturn)) void task_exit(void) {
  asm volatile("cli");
  Task *t = &tasks[current_task];
  serial_fstring("[TASK] Exiting: {str}\n", t->name);
  // Switch back to kernel page table before cleaning up
  serial_writeln("task_exit: switching address space");
  switch_address_space(kernel_pml4);

  serial_writeln("task_exit: marking dead");
  t->state = TASK_DEAD;

  // TODO: proper cleanup later need to walk the addr space
  // Things to free: VMAs, ELF segments,stacks, pages mapped into its addr space
  // task name

  kfree(t->stack);
  kfree((void *)t->name);
  serial_writeln("task_exit: about to yield");
  asm volatile("sti");
  yield();
  for (;;)
    asm volatile("hlt");
}

void tasking_init(void) {
  for (int i = 0; i < MAX_TASKS; i++) {
    tasks[i].state = TASK_UNUSED;
    tasks[i].stack = 0;
    tasks[i].name = 0;
  }

  // bootstrap current kernel execution as task 0
  tasks[0].state = TASK_RUNNING;
  tasks[0].slice = TASK_SLICE_DEFAULT;
  tasks[0].name = "kernel-main";
  current_task = 0;
  task_count = 1;

  serial_fstring("[TASK] Tasking initialised\n");
}

void preempt_check(void) {
  if (current_task < 0)
    return;

  if (tasks[current_task].slice == 0) {
    yield();
  } else {
    tasks[current_task].slice--;
  }
}

void debug_tasks(void) {
  // add in a header of the info like name : id : etc
  // need to figure out justifications
  terminal_fstring("===KERNEL TASKS===\n");
  terminal_fstring("CURRENT TICK : {uint}\n", pit_get_ticks());

  int longest_task_name = 0;
  for (int i = 0; i < task_count; i++) {
    Task *t = &tasks[i];
    if (longest_task_name < strlen(t->name))
      longest_task_name = strlen(t->name);
  }

  for (int task_id = 0; task_id < MAX_TASKS; task_id++) {
    Task *t = &tasks[task_id];
    if (t->state == TASK_UNUSED)
      continue;

    u16 name_diff = longest_task_name - strlen(t->name);
    terminal_fstring("{uint} : ", task_id);
    terminal_fstring("{str} ", t->name);
    for (int i = 0; i < name_diff; i++)
      terminal_fstring(" ");
    terminal_fstring(": ");

    char buf[9];
    ticks_to_time(t->start_tick, buf);
    terminal_fstring("{str} : ", buf);

    // terminal_fstring("{str} : SLICE : {uint}\n",
    // task_state_to_str(t->state),
    //                  t->slice);
    terminal_fstring("{str} \n", task_state_to_str(t->state));

    if (t->is_elf)
      debug_vmas(t);
  }
}

void kill_task(u16 task_id) {
  Task *t = &tasks[task_id];
  serial_fstring("[Tasks] Killing task {uint} called {str}", task_id, t->name);
  t->state = TASK_DEAD;
}

const char *task_state_names[] = {
#define X(name) #name,
    TASK_STATE_LIST
#undef X
};
const char *task_state_to_str(enum task_state s) { return task_state_names[s]; }

void task_sleep(u64 ms) {
  asm volatile("cli");

  Task *t = &tasks[current_task];

  t->wakeup_tick = pit_get_ticks() + ms_to_ticks(ms);
  t->state = TASK_SLEEPING;

  asm volatile("sti");

  yield();
}

void wake_sleeping_tasks(void) {
  for (int task_id = 0; task_id < task_count; task_id++) {
    Task *t = &tasks[task_id];
    if (t->state == TASK_SLEEPING) {
      if (pit_get_ticks() >= t->wakeup_tick) {
        t->state = TASK_READY;
      }
    }
  }
}

int task_create_elf(const char *name, void *file) {
  // ptr from userspace is unreliable
  char *name_cpy = kalloc(strlen(name) + 1);
  memcpy(name_cpy, name, strlen(name) + 1);

  int id = find_free_task_slot();
  if (id < 0)
    return -1;
  Task *t = &tasks[id];

  u64 *proc_addr_space = create_address_space();
  if (!proc_addr_space)
    return -1;

  loaded_elf elf;
  if (!load_elf(t, file, &elf, proc_addr_space))
    return -1;

  // kernel stack
  t->stack = kalloc(TASK_STACK_SIZE);

  // user stack
  for (u64 i = 0; i < USER_STACK_PAGES; i++) {
    void *frame = pmm_alloc_frame();
    memset(frame, 0, FRAME_SIZE);
    u64 virt = USER_STACK_TOP - ((i + 1) * FRAME_SIZE);
    map_page(proc_addr_space, virt, virt_to_phys(frame),
             PTE_PRESENT | PTE_WRITE | PTE_USER);
  }
  u64 stack_bottom = USER_STACK_TOP - (USER_STACK_PAGES * FRAME_SIZE);
  add_vma(t, stack_bottom, USER_STACK_TOP);

  u64 sp = ((u64)t->stack + TASK_STACK_SIZE) & ~0xFULL;
  sp -= 8;
  *(u64 *)sp = (u64)task_exit;

  memset(&t->regs, 0, sizeof(t->regs));
  t->regs.rip = (u64)elf_trampoline;
  t->user_rip = elf.entry_addr;

  t->regs.rsp = sp; // kernel for trampoline
  t->user_rsp = USER_STACK_TOP;

  t->regs.cr3 = virt_to_phys((void *)kernel_pml4); // kernel for trampoline
  t->user_cr3 = virt_to_phys((void *)proc_addr_space);

  t->state = TASK_READY;
  t->vma_list = NULL;
  t->slice = TASK_SLICE_DEFAULT;
  t->name = name_cpy;
  t->wakeup_tick = 0;
  t->start_tick = pit_get_ticks();
  t->segment_count = elf.segment_count;
  for (u64 i = 0; i < elf.segment_count; i++) {
    t->segments[i] = elf.segments[i];
  }
  t->is_elf = true;
  serial_fstring("ELF entry = {uint}\n", elf.entry_addr);
  serial_fstring("Creating ELF userspace task called {str} : id={uint}\n",
                 t->name, id);
  if (id >= task_count)
    task_count = id + 1;
  return id;
}
