/*******************************************************************************
 * SOL 2017/2018
 * Chatty
 * Federico Silvestri 559014
 * Si dichiara che il contenuto di questo file e' in ogni sua parte opera
 * originale dell'autore.
 *******************************************************************************/

/**
 * C Posix source definition.
 */
#define _POSIX_C_SOURCE 200809L

#include "worker.h"

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>

#include "connections.h"
#include "message.h"
#include "ops.h"

#include "log.h"
#include "controller.h"
#include "producer.h"
#include "userman.h"

extern int *sockets;

static void prepare_header(message_hdr_t *hdr) {
	memset(hdr, 0, sizeof(message_hdr_t));
	strncpy(hdr->sender, "SERVER", sizeof(hdr->sender));
}

static short int check_header(message_hdr_t hdr) {
	if (strlen(hdr.sender) == 0) {
		return 1;
	}

	return 0;
}

static void worker_user_list(int index) {
	message_data_t reply;
	memset(&reply, 0, sizeof(message_data_t));

	reply.buf = NULL;
	int users_n = userman_get_users(USERMAN_GET_ALL, &reply.buf);

	if (users_n <= 0) {
		log_fatal("Error during getting online users");
		return;
	}
	reply.hdr.len = users_n;

	int w_size = sendData(sockets[index], &reply);

	if (w_size <= 0) {
		log_warn("Client disconnected without 'thanks', %s", strerror(errno));
	}

	free(reply.buf);
}

static void worker_register_user(int index, message_t msg) {
	message_hdr_t hdr_reply;
	prepare_header(&hdr_reply);

	log_trace("Registering user");
	int reg_status = userman_add_user(msg.hdr.sender);

	switch (reg_status) {
	case 0:
		hdr_reply.op = OP_OK;
		log_info("User registered successfully");
		break;
	case 1:
		hdr_reply.op = OP_NICK_ALREADY;
		log_info("Cannot register user, nickname is already registered");
		break;
	default:
		log_error("User registration failed, return value: %d", reg_status);
		hdr_reply.op = OP_FAIL;
		break;
	}

	if (write(sockets[index], &hdr_reply, sizeof(hdr_reply)) <= 0) {
		log_error("Cannot send to socket, %s", strerror(errno));
	}

	if (reg_status == 0) {
		// we need now to send the user list
		worker_user_list(index);
	}
}

static int worker_action_router(int index, message_t msg) {
	switch (msg.hdr.op) {
	case REGISTER_OP:
		worker_register_user(index, msg);
		break;
	case CONNECT_OP:
	case POSTTXT_OP:
	case POSTTXTALL_OP:
	case POSTFILE_OP:
	case GETFILE_OP:
	case GETPREVMSGS_OP:
	case USRLIST_OP:
	case UNREGISTER_OP:
	case DISCONNECT_OP:
		log_fatal("NOT IMPLEMENTED ACTIONS");
		break;
	case CREATEGROUP_OP:
	case ADDGROUP_OP:
	case DELGROUP_OP:
		// optional
		break;
	default:
		log_error("Operation not recognized by server, bad protocol.");
		return -1;

	}

	return 0;
}

void worker_run(amqp_message_t message) {
	// access data pointed by body message, i.e. fd
	int *udata = (int*) message.body.bytes;
	int index = udata[0];

	message_t msg;
	int read_size;

	read_size = readMsg(sockets[index], &msg);

	if (read_size == 0) {
		producer_disconnect_host(index);
		// no other operation are possible on socket.
		return;
	}

	if (check_header(msg.hdr) == 1) {
		log_warn("Someone has sent anonymous message, rejecting");
		producer_disconnect_host(index);
		// no other operation are possible on socket.
		return;
	}

	int ra_ret = worker_action_router(index, msg);

	if (ra_ret == -1) {
		// grave problem
		producer_disconnect_host(index);
		return;
	}

	producer_unlock_socket(index);
	// no other operation are possible on socket.
}
