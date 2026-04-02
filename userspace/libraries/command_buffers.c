/* Command buffers implemented in userspace ontop of the 
   MailboxMessage and shared memory APIs.
 
   This should probably be done in the kernel but this is ok for now
 
   API:
   On server: listen(0) or int cmdbuf_id = listen(-1) for auto id
   On client: char *buf = connect(0)
   On server: char *buf = accept(0, -1)
 
   If CMDBUF_BLOCK is passed as the second argument to accept(), 
   it will block until a client connects to the server */
#include "basic.h"
#include "lykosapi.h"

//@Todo user configurable
#define CMDBUF_SIZE 4096

enum cmdbuf_msg_type {
	CMDB_connect,
	CMDB_accept,
};

enum cbdbuf_msg_flags {
	CMDB_bidirectional,
	CMDB_block,
	CMDB_not_valid_here,
};

typedef struct cmdbuf_msg {
	enum cmdbuf_msg_type type;
	union {
		int mailboxid;
		struct {
			int shm_id;
			size_t cmdbuf_size;
		};
	};
	u32 flags;
} cmdbuf_msg;

typedef struct cmdbuf_bidirectional {
	struct {
		void *buffer;
		int write_head;
		int read_head;
		ssize sz;
	} buffers[2];
	ssize sz;

} cmdbuf_bidirectional;


atomic bool cmdbuf_client_init = false;
int cmdbuf_client_mboxid = 0;

void init_cmdbuf_client() {
	cmdbuf_client_mboxid = mbox_create(-1);
	cmdbuf_client_init = true;
	return;
}

int cmdbuf_listen(int bufid) {
	int id = mbox_create(bufid);
	return id;
}

void *cmdbuf_connect(int bufid, int *cmdbuf_size) {
	if (!cmdbuf_client_init) {
		init_cmdbuf_client();
	}
	cmdbuf_msg msg;
	MailboxMessage out;

	msg.type = CMDB_connect;
	msg.mailboxid = cmdbuf_client_mboxid;
	mbox_send(bufid, &msg, sizeof msg);

	memset(&msg, 0, sizeof msg);
	//todo timeout
	while (!mbox_receive(bufid, &out)) {
		sleep(1);
		continue;
	}
	if (msg.type == CMDB_accept) {
		*cmdbuf_size = msg.cmdbuf_size;
		size_t shmsize = 0;
		return shm_map(msg.shm_id, &shmsize);
	}

	return null;
}

int cmdbuf_connect_bidirectional(int bufid, int *cmdbuf_size, cmdbuf_bidirectional *ret) {
	if (!cmdbuf_client_init) {
		init_cmdbuf_client();
	}
	int id = shm_create(CMDBUF_SIZE, true);
	cmdbuf_msg msg;
	MailboxMessage out;

	msg.type = CMDB_connect;
	msg.flags = CMDB_bidirectional;
	msg.mailboxid = cmdbuf_client_mboxid;
	msg.shmid = id;
	msg.cmdbuf_size = CMDBUF_SIZE;
	mbox_send(bufid, &msg, sizeof msg);

	memset(&msg, 0, sizeof msg);
	//todo timeout
	while (!mbox_receive(bufid, &out)) {
		sleep(1);
		continue;
	}
	if (msg.type == CMDB_accept) {
		*cmdbuf_size = msg.cmdbuf_size;
		size_t shmsize = 0;
		ret->cmdbuffer[0] = shm_map(id, &shmsize);
		ret->cmdbuffer[1] = shm_map(msg.shm_id, &shmsize);
		ret->sz = CMDBUF_SIZE;
		return 0;
	}

	return -1;
}

void *cmdbuf_handle_client_connect(cmdbuf_msg *msg) {
	int id = shm_create(CMDBUF_SIZE, true);
	cmdbuf_msg response;
	response.type = CMDB_accept;
	response.shm_id = id;
	response.cmdbuf_size = CMDBUF_SIZE;

	mbox_send(msg->mailboxid, &response, sizeof response);

	size_t sz;
	return shm_map(id, &sz);
}

int cmdbuf_handle_client_connect_bidirectional(int bufid, cmdbuf_msg *msg, cmdbuf_bidirectional *ret) {
	int id = shm_create(CMDBUF_SIZE, true);
	int client_id = -1;
	ret->cmdbuffer[0] = shm_map(id, &shmsize);
	ret->cmdbuffer[1] = shm_map(msg.shm_id, &shmsize);
	ret->sz = CMDBUF_SIZE;

	cmdbuf_msg response;
	response.type  = CMDB_accept;
	response.flags = CMDB_bidirectional;
	response.shm_id = id;
	response.cmdbuf_size = CMDBUF_SIZE;

	mbox_send(msg->mailboxid, &response, sizeof response);
	return 0;
}


void cmdbuf_deny_client(cmdbuf_msg *msg) {
	cmdbuf_msg response = {0};
	response.type = CMDB_deny;
	response.flags |= CMDB_not_valid_here;
	mbox_send(msg->mailboxid, &response, sizeof response);

	return;
}

void *cmdbuf_accept(int bufid, int argflags) {
	MailboxMessage out;
	cmdbuf_msg msg;
	memset(&msg, 0, sizeof msg);
	bool block          = argflags & CMDBUF_block;
	bool bidirectional  = argflags & CMDBUF_bidirectional;
	while (1) {
		//todo kernel sleep
		int ret = mbox_receive(bufid, &out);
		if (!ret && block) {
			sleep(1);
			continue;
		} else if (!ret && !block) {
			break;
		}

		msg = *(cmdbuf_msg *) &out;
		//if valid
		switch (msg.type) {
		case CMDB_connect:
			if ((msg.flags & CMDB_bidirectional) != bidirectional) {
				cmdbuf_deny_client(&msg);
				return null;
			}
			if (!bidirectional) return cmdbuf_handle_client_connect(&msg);
			else return cmdbuf_handle_client_connect_bidirectional(&msg);
		default: break;
		}
	}
	return null;
}

int cmdbuf_accept_bidirectional(int bufid, u32 flags, cmdbuf_bidirectional *ret) {
	MailboxMessage out;
	cmdbuf_msg msg;
	memset(&msg, 0, sizeof msg);
	bool block = flags & CMDB_block;
	while (1) {
		//todo kernel sleep
		int ret = mbox_receive(bufid, &out);
		if (!ret && block) {
			sleep(1);
			continue;
		} else if (!ret && !block) {
			break;
		}

		msg = *(cmdbuf_msg *) &out;
		//if valid
		switch (msg.type) {
		case CMDB_connect:
			if (!(msg.flags & CMDB_bidirectional)) {
				cmdbuf_deny_client(&msg);
				return null;
			}
			return cmdbuf_handle_client_connect_bidirectional(&msg, ret);
		default: break;
		}
	}
	return null;
}

//Message api

typedef struct cmdbuf_message_buffer {
	atomic u32 buffer_index;
	ssize sz;
	u8 *buf;
} cmdbuf_message_buffer;

void cmdbuf_sendmsg(cmdbuf_message_buffer *msgbuf, void *data, ssize sz) {
//	if (!atomic_compare_exchange_strong(msgbuf->lock, false, true)) {
//		sleep(1);
//		return;
//	}
	int offset = msg->buffer_index;
	msg->buffer_index += sz;
	memcpy(msgbuf->buf + offset, data, sz);

	msgbuf->lock = false;
	return;
}
