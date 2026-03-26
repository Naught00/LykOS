#pragma once
#include "arch/x86_64/idt.h"
#include "req.h"


i64 sys_exit(interrupt_frame *frame);
i64 sys_sleep(interrupt_frame *frame);
i64 sys_write(interrupt_frame *frame);
u64 map_fb(interrupt_frame *frame);
i64 get_key_event(interrupt_frame *frame);
u64 sys_mmap(interrupt_frame *frame);
i64 sys_exec(interrupt_frame *frame);
i64 sys_mbox_create(interrupt_frame *frame);
i64 sys_mbox_send(interrupt_frame *frame);
i64 sys_mbox_receive(interrupt_frame *frame);
i32 sys_shm_create(interrupt_frame *frame);
u64 sys_shm_map(interrupt_frame *frame);
