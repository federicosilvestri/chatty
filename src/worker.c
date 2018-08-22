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

static void prepare_data(message_data_t *msg, char *nickname) {
	memset(msg, 0, sizeof(message_data_t));
	msg->buf = NULL;
	strncpy(msg->hdr.receiver, nickname, MAX_NAME_LENGTH);
	msg->hdr.len = 0;
}

static short int check_header(message_hdr_t *hdr) {
	if (strlen(hdr->sender) == 0) {
		return 1;
	}

	return 0;
}

static void worker_user_list(int index, message_t *msg) {
	message_data_t reply;
	prepare_data(&reply, msg->hdr.sender);

	reply.buf = NULL;
	size_t list_size = userman_get_users(USERMAN_GET_ALL, &reply.buf);

	if (list_size <= 0) {
		log_fatal("Error during getting online users");
		return;
	}

	reply.hdr.len = (unsigned int) list_size;

	int w_size = sendData(sockets[index], &reply);
	if (w_size <= 0) {
		log_fatal("Cannot send data to client!", strerror(errno));
	}

	free(reply.buf);
}

static void worker_register_user(int index, message_t *msg) {
	message_hdr_t hdr_reply;
	prepare_header(&hdr_reply);

	log_trace("Registering user");
	int reg_status = userman_add_user(msg->hdr.sender);

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

	if (sendHeader(sockets[index], &hdr_reply) <= 0) {
		log_error("Cannot send to socket, %s", strerror(errno));
	}

	if (reg_status == 0) {
		// we need now to send the user list
		worker_user_list(index, msg);
	}
}

static void worker_deregister_user(int index, message_t *msg) {
	message_hdr_t hdr_reply;
	prepare_header(&hdr_reply);

	log_trace("DEregistering user");
	int dereg_status = 0; // userman_delete_user(msg.hdr.sender);

	switch (dereg_status) {
	case 0:
		hdr_reply.op = OP_OK;
		log_info("User DEregistered successfully");
		break;
	case 1:
		hdr_reply.op = OP_NICK_UNKNOWN;
		log_info("Cannot DEregister user, nickname is not registered");
		break;
	default:
		log_error("User DEregistration failed, return value: %d", dereg_status);
		hdr_reply.op = OP_FAIL;
		break;
	}

	if (sendHeader(sockets[index], &hdr_reply) <= 0) {
		log_error("Cannot send to socket, %s", strerror(errno));
	}

	msg->hdr.sender[0] = 's';
}


static inline void worker_disconnect_user(int index, message_t *msg) {
	// update status to userman
	if (userman_set_user_status(msg->hdr.sender, USERMAN_STATUS_OFFL) == false) {
		log_fatal("Cannot set user offline due to previous errors!");
	}

	producer_disconnect_host(index);
}

static inline void worker_connect_user(int index, message_t *msg) {
	// first check if user exists
	if (userman_user_exists(msg->hdr.sender) == true) {
		// connect it
		if (userman_set_user_status(msg->hdr.sender, USERMAN_STATUS_ONL) == false) {
			log_fatal("Cannot set user status due to previous error!");
			return;
		}
		// user connected
	} else {
		// user is not registered, write back response
		message_hdr_t reply;
		prepare_header(&reply);

		reply.op = OP_NICK_UNKNOWN;

		if (sendHeader(sockets[index], &reply) <= 0) {
			log_error("Cannot send reply to client!");
		}

		// done
	}
}

static int worker_action_router(int index, message_t *msg) {
	int ret;

	log_trace("ACTION REQUESTED: %d", msg->hdr.op);

	switch (msg->hdr.op) {
	case REGISTER_OP:
		worker_register_user(index, msg);
		ret = 0;
		break;
	case CONNECT_OP:
		worker_connect_user(index, msg);
		ret = 0;
		break;
	case POSTTXT_OP:
	case POSTTXTALL_OP:
	case POSTFILE_OP:
	case GETFILE_OP:
	case GETPREVMSGS_OP:
	case USRLIST_OP:
		log_fatal("NOT IMPLEMENTED ACTIONS");
		ret = -1;
		break;
	case UNREGISTER_OP:
		worker_deregister_user(index, msg);
		ret = 2;
		break;
	case DISCONNECT_OP:
		worker_disconnect_user(index, msg);
		ret = 1;
		break;
	case CREATEGROUP_OP:
	case ADDGROUP_OP:
	case DELGROUP_OP:
		// optional
		log_fatal("OPTIONAL NOT IMPLEMENTED");
		ret = -1;
		break;
	default:
		log_error("Operation not recognized by server, bad protocol.");
		ret = -1;
		break;
	}

	return ret;
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

	if (check_header(&msg.hdr) == 1) {
		log_warn("Someone has sent anonymous message, rejecting");
		producer_disconnect_host(index);
		// no other operation are possible on socket.
		return;
	}

	int ra_ret = worker_action_router(index, &msg);

	switch (ra_ret) {
	case -1:
		// fatal problem, disconnect
		worker_disconnect_user(index, &msg);
		break;
	case 0:
		// success, release the socket
		worker_disconnect_user(index, &msg);
		// no other operation are possible on socket.
		break;
	case 1:
		// user is already disconnect or socket is already released
		break;
	case 2:
		// connection must be closed, but user is not registered
		producer_disconnect_host(index);
		break;
	default:
		log_fatal(
				"Programming error, unexpected return value of action router");
		break;
	}
}
