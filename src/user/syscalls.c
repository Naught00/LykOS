#include "user/syscalls.h"
#include "arch/x86_64/idt.h"
#include "drivers/ps2.h"
#include "drivers/serial.h"
#include "graphics/draw.h"
#include "mem/mem.h"
#include "proc/mailbox.h"
#include "proc/shm.h"
#include "proc/task.h"
#include "terminal.h"
#include "user/exec.h"
#include "user/input.h"

i64 sys_exit(interrupt_frame *frame) {
  // TODO: use frame for reason why sys exit e.g success failure etc
  (void)frame;
  task_exit();
  return 1;
}

i64 sys_sleep(interrupt_frame *frame) {
  u64 ms = frame->rdi;
  serial_fstring("Sleeping by syscall for {uint} ms\n", ms);
  task_sleep(ms);
  return 1;
}

i64 sys_write(interrupt_frame *frame) {
  char *str = (char *)frame->rdi;
  terminal_fstring("{str}", str);
  serial_writeln(str);
  return 1;
}

u64 map_fb(interrupt_frame *frame) {
  (void)frame;
  u64 fb_vaddr = 0xFB000000;
  limine_framebuffer fb = *get_framebuffer();
  u64 fb_phys_addr = virt_to_phys(fb.address);
  u64 fb_size = fb.width * fb.height * sizeof(u32);

  Task *t = &tasks[current_task];
  u64 proc_cr3 = t->user_cr3;

  // TODO: look into bigger pages, for now just using smallest
  u64 pages_needed = ALIGN_UP(fb_size, PAGE_SIZE) / PAGE_SIZE;

  u64 flags = PTE_PRESENT | PTE_WRITE | PTE_USER | PTE_WRITE_THROUGH;
  for (uint64_t i = 0; i < pages_needed; i++) {
    uint64_t offset = i * PAGE_SIZE;
    serial_fstring("map fb {uint}\n", i);
    map_page(phys_to_virt(proc_cr3), fb_vaddr + offset, fb_phys_addr + offset,
             flags);
  }

  return fb_vaddr;
}

i64 get_key_event(interrupt_frame *frame) {
  KeyEvent *ev_ptr = (KeyEvent *)frame->rdi;
  if (key_event_tail == key_event_head) {
    return 0;
  }
  *ev_ptr = key_event_buffer[key_event_tail];
  key_event_tail = (key_event_tail + 1) % KEYBOARD_BUFFER_SIZE;
  return 1;
}

// TODO: on demand allocation instead of doing it upfront, kernel stalls in
// syscall if allocation is large
u64 sys_mmap(interrupt_frame *frame) {
  Task *t = &tasks[current_task];
  u64 size = frame->rdi;

  u64 addr = find_free_region(t, size);
  if (addr == 0) {
    serial_fstring("MMAP error\n");
    return 0;
  }

  u64 pages = ALIGN_UP(size, PAGE_SIZE) / PAGE_SIZE;
  for (u64 i = 0; i < pages; i++) {
    void *frame = pmm_alloc_frame();
    memset(frame, 0, FRAME_SIZE);
    u64 virt = addr + (i * FRAME_SIZE);
    map_page(phys_to_virt(t->user_cr3), virt, virt_to_phys(frame),
             PTE_PRESENT | PTE_WRITE | PTE_USER);
  }

  add_vma(t, addr, addr + (pages * FRAME_SIZE));
  return addr;
}

u64 map_key_events(interrupt_frame *frame) {
  (void)frame;
  Task *t = &tasks[current_task];
  u64 proc_cr3 = t->user_cr3;

  u64 addr = (u64)key_event_buffer;
  u64 buf_size = KEYBOARD_BUFFER_SIZE * sizeof(KeyEvent);

  u64 pages_needed = ALIGN_UP(buf_size, PAGE_SIZE) / PAGE_SIZE;

  u64 flags = PTE_PRESENT | PTE_WRITE | PTE_USER;

  for (uint64_t i = 0; i < pages_needed; i++) {
    uint64_t offset = i * PAGE_SIZE;
  }

  add_vma(t, addr, addr + (pages_needed * FRAME_SIZE));
  return 1;
}

i64 sys_exec(interrupt_frame *frame) {
  char *file_name = (char *)frame->rdi;
  i64 ret = exec(file_name);
  return ret;
}

i64 sys_mbox_create(interrupt_frame *frame) {
  u64 requested_id = frame->rdi;
  return mbox_create(requested_id);
}

i64 sys_mbox_send(interrupt_frame *frame) {
  serial_writeln("in sysmboxsend");
  u64 id = frame->rdi;
  char *data = (char *)frame->rsi;
  u64 data_len = frame->rdx;
  return mbox_send(id, data, data_len);
}

i64 sys_mbox_receive(interrupt_frame *frame) {
  u64 id = frame->rdi;
  MailboxMessage *out = (MailboxMessage *)frame->rsi;
  return mbox_receive(id, out);
}

i32 sys_shm_create(interrupt_frame *frame) {
  return shm_create(frame->rdi, frame->rsi);
}

u64 sys_shm_map(interrupt_frame *frame) {
  return shm_map(frame->rdi, (u64 *)frame->rsi);
}
