#include "mailbox.h"
#include "drivers/serial.h"
#include "proc/task.h"

Mailbox mailboxes[MAX_MAILBOXES];

i64 mbox_create(i64 requested_id) {
  // auto assign
  if (requested_id < 0) {
    for (int i = 0; i < MAX_MAILBOXES; i++) {
      if (!mailboxes[i].is_active) {
        requested_id = i;
        break;
      }
    }
  }
  Mailbox *mbox = &mailboxes[requested_id];

  if (mbox->is_active) {
    serial_writeln("[MBOX CREATE] Mailbox taken");
    return -1;
  }

  *mbox = (Mailbox){
      .is_active = true,
      .id = requested_id,
      .owner_pid = current_task,
      .tail = 0,
      .count = 0,
  };

  return requested_id;
}
i64 mbox_send(u64 mailbox_id, char *data, u64 data_len) {
  if (data_len > MAILBOX_MESSAGE_SIZE) {
    serial_writeln("[MBOX SEND] Too much data");
    return -1;
  }
  if (mailbox_id < 0 || mailbox_id > MAX_MAILBOXES) {
    serial_writeln("[MBOX SEND] Invalid ID");
    return -2;
  }

  Mailbox *mbox = &mailboxes[mailbox_id];

  if (!mbox->is_active) {
    serial_writeln("[MBOX SEND] Not active");
    return -3;
  }

  if (mbox->count == MAILBOX_CAPACITY) {
    serial_writeln("[MBOX SEND] Mailbox full");
    return -4;
  }

  MailboxMessage *msg = &mbox->queue[mbox->tail];
  memcpy(msg->data, data, data_len);
  msg->len = data_len;

  mbox->queue[mbox->tail] = *msg;
  mbox->tail = (mbox->tail + 1) % MAILBOX_CAPACITY;
  mbox->count++;
  if (mbox->sleeping_pid) {
    tasks[mbox->sleeping_pid].state = TASK_READY;
    serial_fstring("Waking up task {uint}\n", mbox->sleeping_pid);
  }
  serial_fstring("[MBOX] Count {uint}\n", mbox->count);

  // wakeup mbox->sleeping_pid

  serial_writeln("sent to mailbox!");
  return 1;
}

i64 mbox_receive(u64 mailbox_id, MailboxMessage *out, bool blocking) {
  if (mailbox_id < 0 || mailbox_id > MAX_MAILBOXES) {
    serial_writeln("[MBOX RECEIVE] Invalid ID");
    return -1;
  }

  Mailbox *mbox = &mailboxes[mailbox_id];

  if (current_task != mbox->owner_pid) {
    serial_writeln("[MBOX RECEIVE] Invalid permissions");
    return -2;
  }

  while (mbox->count == 0) {
    serial_fstring("[MBOX RECEIVE] Mailbox empty");

    if (blocking) {
      serial_fstring("proc {str} blocking on mailbox {uint}\n",
                     tasks[current_task].name, mailbox_id);
      mbox->sleeping_pid = current_task;
      tasks[current_task].state = TASK_BLOCKED;
      yield();
    } else {
      return 0;
    }
  }

  *out = mbox->queue[mbox->head];
  mbox->head = (mbox->head + 1) % MAILBOX_CAPACITY;
  mbox->count--;
  serial_fstring("[MBOX] Count {uint}\n", mbox->count);

  return 1;
}

i64 mbox_delete(u64 mailbox_id);
