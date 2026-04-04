#pragma once
#include "req.h"
#include <stdbool.h>

#define MAX_MAILBOXES 128
#define MAILBOX_CAPACITY 12
#define MAILBOX_MESSAGE_SIZE 128

typedef struct {
  u64 sender_pid;
  u64 len;
  char data[MAILBOX_MESSAGE_SIZE];
} MailboxMessage;

typedef struct {
  bool is_active;

  u64 id;
  i64 owner_pid;
  bool is_blocked;

  u64 head;
  u64 tail;

  u64 count;
  MailboxMessage queue[MAILBOX_CAPACITY];

} Mailbox;

extern Mailbox mailboxes[MAX_MAILBOXES];

i64 mbox_create(i64 mailbox_id);
i64 mbox_send(u64 mailbox_id, char *data, u64 data_len);
i64 mbox_receive(u64 mailbox_id, MailboxMessage *out, bool blocking);
i64 mbox_delete(u64 mailbox_id);
void free_mailboxes(i64 owner_pid);
