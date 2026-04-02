#include "idt.h"
#include "arch/x86_64/io.h"
#include "drivers/pit.h"
#include "drivers/ps2.h"
#include "drivers/serial.h"
#include "mem/mem.h"
#include "proc/task.h"
#include "terminal.h"
#include "user/syscalls.h"
#include <stdbool.h>
#include <stdint.h>

static IDTEntry idt[256];
static IDTPointer idt_ptr;

#define RING_0_64_PRESENT_GATE 0x8E
#define RING_3_64_PRESENT_GATE 0xEE
#define KERNEL_CODE_SEGMENT 0x08

extern void isr_stub_0(void);
extern void isr_stub_13(void);
extern void isr_stub_14(void);
extern void isr_stub_32(void);
extern void isr_stub_33(void);
extern void isr_stub_60(void);
extern void isr_stub_128(void);
// the array isr_stubs.asm is using
isr_handler_fn isr_handler_table[256] = {0};

void register_interrupt(uint8_t vector, isr_handler_fn handler) {
  isr_handler_table[vector] = handler;
}

void set_idt_gate(u32 index, u64 handler_ptr, u16 selector, u8 flags) {

  idt[index].offset_low = handler_ptr & 0xFFFF;
  idt[index].selector = selector; // code segment selector to GDT, usually 0x08
  idt[index].ist = 0;
  idt[index].type_attr = flags;
  idt[index].offset_mid = (handler_ptr >> 16) & 0xFFFF;
  idt[index].offset_high = (handler_ptr >> 32) & 0xFFFFFFFF;
  idt[index].zero = 0;
}

static inline bool are_interrupts_enabled(void) {
  unsigned long flags;
  asm volatile("pushf\n\t"
               "pop %0"
               : "=g"(flags));
  return flags & (1 << 9);
}

void idt_init(void) {
  serial_writeln("Initialising interrupt descriptor table");
  idt_ptr.limit = (sizeof(IDTEntry) * 256) - 1;
  idt_ptr.base = (u64)&idt;

  set_idt_gate(0, (u64)isr_stub_0, KERNEL_CODE_SEGMENT, RING_0_64_PRESENT_GATE);

  set_idt_gate(13, (u64)isr_stub_13, KERNEL_CODE_SEGMENT,
               RING_0_64_PRESENT_GATE);
  set_idt_gate(14, (u64)isr_stub_14, KERNEL_CODE_SEGMENT,
               RING_0_64_PRESENT_GATE);

  set_idt_gate(32, (u64)isr_stub_32, KERNEL_CODE_SEGMENT,
               RING_0_64_PRESENT_GATE);

  set_idt_gate(33, (u64)isr_stub_33, KERNEL_CODE_SEGMENT,
               RING_3_64_PRESENT_GATE);

  set_idt_gate(60, (u64)isr_stub_60, KERNEL_CODE_SEGMENT,
               RING_3_64_PRESENT_GATE);

  set_idt_gate(128, (u64)isr_stub_128, KERNEL_CODE_SEGMENT,
               RING_3_64_PRESENT_GATE);

  register_interrupt(0, isr_divide_error);
  register_interrupt(13, isr_gpf);
  register_interrupt(14, isr_pagefault);
  register_interrupt(32, isr_pit);
  register_interrupt(33, isr_ps2_keyboard);
  register_interrupt(60, isr_test);
  register_interrupt(128, isr_syscall);

  asm volatile("lidt %0" : : "m"(idt_ptr));

  if (are_interrupts_enabled()) {
    serial_writeln("Interrupts enabled");
  } else {
    serial_writeln("Interrutps not enabled");
  }
}

void isr_divide_error(interrupt_frame *frame) {
  (void)frame;
  serial_writeln("Divide by zero!");
}

void isr_test(interrupt_frame *frame) {
  (void)frame;
  terminal_fstring("Hello from int 60!\n");
}

void debug_frame(interrupt_frame *frame) {
  serial_fstring("RIP 0x{hex}\n", frame->rip);
  serial_fstring("RIP {uint}\n", frame->rip);
  debug_cr3();
}

void isr_gpf(interrupt_frame *frame) {
  serial_fstring("GPF Exception in task id {uint} : {str}, terminating task\n",
                 current_task, tasks[current_task].name);
  terminal_fstring(
      "GPF Exception in task id {uint} : {str}, terminating task\n",
      current_task, tasks[current_task].name);
  tasks[current_task].state = TASK_DEAD;
  debug_frame(frame);
  end_of_interrupt();
  yield();
}

void isr_pit(interrupt_frame *frame) {
  (void)frame;
  pit_tick();
  serial_fstring("tick pit 876\n");

  // debug, laggy
  // serial_fstring("timer cr3 = {uint}\n", cr3);
  // serial_fstring("kernel cr3 = {uint}\n", virt_to_phys((void *)kernel_pml4));
  wake_sleeping_tasks();

  if (current_task >= 0) {
    Task *t = &tasks[current_task];

    if (t->state == TASK_RUNNING && t->slice > 0) {
      serial_fstring("decrementing timeslice of {str}\n", t->name);
      t->slice--;

      if (t->slice == 0) {
        t->slice = TASK_SLICE_DEFAULT;
        serial_fstring("Pre-empting yield\n");
        end_of_interrupt();
        // asm volatile("sti");
        yield();
      }
    }
  }
  end_of_interrupt();
}

void isr_pagefault(interrupt_frame *frame) {
  serial_fstring("Page fault!\n");
  u64 cr2;
  asm volatile("mov %%cr2, %0" : "=r"(cr2));
  serial_writeln("=== PAGE FAULT ===");
  serial_fstring("  faulting address (CR2) = {hex}\n", cr2);
  serial_fstring("  rip = {hex}\n", frame->rip);
  serial_fstring("  rsp = {hex}\n", frame->rsp);
  serial_fstring("  error code = {uint}\n", frame->error_code);
  serial_fstring("  rax = {hex}\n", frame->rax);
  serial_fstring("  rbx = {hex}\n", frame->rbx);
  serial_fstring("  rcx = {hex}\n", frame->rcx);
  serial_fstring("  rdx = {hex}\n", frame->rdx);
  serial_fstring("  current task = {uint}\n", current_task);
  serial_fstring("  task name = {str}\n", tasks[current_task].name);
  u64 *stack = (u64 *)frame->rsp;
  serial_writeln("Stack dump:");
  for (int i = 0; i < 16; i++) {
    serial_fstring("  [{uint}] = {hex}\n", (u64)i, stack[i]);
  }
  debug_frame(frame);
  terminal_fstring("Process page fault, exiting..\n");
  task_exit();
}

void isr_syscall(interrupt_frame *frame) {
  switch (frame->rax) {
  case 0:
    frame->rax = sys_exit(frame);
    break;
  case 1:
    frame->rax = sys_sleep(frame);
    break;
  case 2:
    frame->rax = sys_write(frame);
    break;
  case 3:
    frame->rax = map_fb(frame);
    break;
  case 4:
    frame->rax = get_key_event(frame);
    break;
  case 5:
    frame->rax = sys_mmap(frame);
    break;
  case 6:
    frame->rax = sys_exec(frame);
    break;
  case 7:
    frame->rax = sys_mbox_create(frame);
    break;
  case 8:
    frame->rax = sys_mbox_send(frame);
    break;
  case 9:
    frame->rax = sys_mbox_receive(frame);
    break;
  case 10:
    frame->rax = sys_shm_create(frame);
    break;
  case 11:
    frame->rax = sys_shm_map(frame);
    break;
  case 12:
    frame->rax = map_key_events(frame);
    break;
  }
}
