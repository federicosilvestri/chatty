/*******************************************************************************
 * SOL 2017/2018
 * Chatty
 * Federico Silvestri 559014
 * Si dichiara che il contenuto di questo file e' in ogni sua parte opera
 * originale dell'autore.
 *******************************************************************************/
/**
 * This file is the worker component of the architecture.
 * The worker contains socket-synchronous functions to process
 * each single message sent by producer and received by consumer.
 * Each function is thread-safe for definition.
 * @file worker.c
 */

/**
 * C Posix source definition.
 */
#define _POSIX_C_SOURCE 200809L

#include "worker.h"

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <libconfig.h>

#include "connections.h"
#include "message.h"
#include "ops.h"

#include "log.h"
#include "config.h"
#include "controller.h"
#include "producer.h"
#include "userman.h"

/**
 * Variable to register initialization status.
 */
static bool init = false;

/**
 * External configuration by configuration header.
 */
extern config_t server_conf;

/**
 * External sockets array.
 */
extern int *sockets;

/**
 * How many connections producer is able to support.
 */
extern int producer_max_connections;

/**
 * Internal max_message_size configuration parameter.
 */
static int max_message_size;

/**
 * Internal max_file_size configuration parameter.
 */
static int max_file_size;

/**
 * Internal max_hist_msgs configuration parameter.
 */
static int max_hist_msgs;

/**
 * Internal max_concurrent_user_connections_n parameter.
 */
static int max_concurrent_user_connections_n;

/**
 * Internal file_dir_name configuration parameter.
 */
static const char* file_dir_name;

bool worker_init() {
	if (init == true) {
		log_error("Worker is already initialized!");
		return false;
	}

	if (config_lookup_int(&server_conf, "MaxMsgSize",
			&max_message_size) == CONFIG_FALSE) {
		log_fatal("Cannot get value for MaxMsgSize!");
		return false;
	}

	if (max_message_size <= 0) {
		log_fatal("MaxMsgSize cannot be <= 0");
		return false;
	}

	if (config_lookup_int(&server_conf, "MaxFileSize",
			&max_file_size) == CONFIG_FALSE) {
		log_fatal("Cannot get value for MaxFileSize!");
		return false;
	}

	if (max_file_size <= 0) {
		log_fatal("MaxFileSize cannot be <= 0");
		return false;
	}

	if (config_lookup_int(&server_conf, "MaxHistMsgs",
			&max_hist_msgs) == CONFIG_FALSE) {
		log_fatal("Cannot get value for MaxHistMsgs!");
		return false;
	}

	if (max_hist_msgs < 0) {
		log_fatal("MaxHistsMsg cannot be < 0!");
		return false;
	}

	if (config_lookup_string(&server_conf, "DirName",
			&file_dir_name) == CONFIG_FALSE) {
		log_fatal("Cannot get value for DirName!");
		return false;
	}

	if (config_lookup_int(&server_conf, "MaxConcurrentConnectionPerUser",
			&max_concurrent_user_connections_n) == CONFIG_FALSE) {
		log_fatal("Cannot get value for MaxConcurrentConnectionPerUser");
		return false;
	}

	if (max_concurrent_user_connections_n < 1
			|| max_concurrent_user_connections_n > producer_max_connections) {
		log_fatal(
				"MaxConcurrentConnectionPerUser cannot be < 1 or > MaxConnections!");
		return false;
	}

	init = true;

	return true;
}

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

static bool check_connection(int index, message_t *msg) {
	// check the sender
	if (strlen(msg->hdr.sender) == 0) {
		log_warn("Someone has sent anonymous message, rejecting");
		// disconnect client brutally
		producer_disconnect_host(index);
		// no other operation are possible on socket.
		return false;
	}

	// check concurrent connection
	int user_conc_conn = producer_get_fds_n_by_nickname(msg->hdr.sender) + 1;
	if (user_conc_conn > max_concurrent_user_connections_n) {
		log_warn(
				"Exceeded concurrent connections per user! conns=%d, maxconcuconn=%d, user=%s",
				user_conc_conn, max_concurrent_user_connections_n,
				msg->hdr.sender);
		// disconnect client brutally
		producer_disconnect_host(index);
		// no other operation are possible on socket.
		return false;
	}

	return true;
}

static void worker_user_list(int index, message_t *msg, bool ack, char type) {
	// if ack is set to ok, it sends ack message
	if (ack == true) {
		message_hdr_t reply_hdr;
		prepare_header(&reply_hdr);

		reply_hdr.op = OP_OK;
		if (sendHeader(sockets[index], &reply_hdr) <= 0) {
			log_error("Cannot send ACK to client");
			return;
		}
	}

	message_data_t reply;
	prepare_data(&reply, msg->hdr.sender);

	reply.buf = NULL;
	size_t list_size = userman_get_users(type, &reply.buf);

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
		worker_user_list(index, msg, false, USERMAN_GET_ONL);
	}
}

static void worker_deregister_user(int index, message_t *msg) {
	message_hdr_t hdr_reply;
	prepare_header(&hdr_reply);

	log_trace("DEregistering user");
	bool dereg_status = userman_delete_user(msg->hdr.sender);

	if (dereg_status == true) {

		hdr_reply.op = OP_OK;
		log_info("User DEregistered successfully");
	} else {
		log_error("User DEregistration failed, maybe user does not exist");
		hdr_reply.op = OP_FAIL;
	}

	if (sendHeader(sockets[index], &hdr_reply) <= 0) {
		log_error("Cannot send to socket, %s", strerror(errno));
	}
}

static inline void worker_disconnect_user(int index, message_t *msg) {
	// check if another instance of user is connected
	int opened_sockets = producer_get_fds_n_by_nickname(msg->hdr.sender);

	// more than one connection
	if (opened_sockets == 1) {
		// update status to userman
		if (userman_set_user_status(msg->hdr.sender,
		USERMAN_STATUS_OFFL) == false) {
			log_fatal("Cannot set user offline due to previous errors!");
		}
	} else if (opened_sockets <= 0) {
		log_fatal("Current user is %s, but opened sockets are %d.",
				msg->hdr.sender, opened_sockets);
	}

	producer_disconnect_host(index);
}

static inline void worker_connect_user(int index, message_t *msg) {
	message_hdr_t reply;
	prepare_header(&reply);

	// first check if user exists
	if (userman_user_exists(msg->hdr.sender) == true) {
		// connect it
		if (userman_set_user_status(msg->hdr.sender,
		USERMAN_STATUS_ONL) == false) {
			log_fatal("Cannot set user status due to previous error!");
			return;
		}

		// user connected, set to the cache
		producer_set_fd_nickname(index, msg->hdr.sender);

		// send ACK
		reply.op = OP_OK;

		if (sendHeader(sockets[index], &reply) <= 0) {
			log_error("Cannot send reply to client!");
		}

		// send user list
		worker_user_list(index, msg, false, USERMAN_GET_ONL);
	} else {
		// user is not registered, write back response
		reply.op = OP_NICK_UNKNOWN;

		if (sendHeader(sockets[index], &reply) <= 0) {
			log_error("Cannot send reply to client!");
		}

		// done
	}
}

static bool worker_send_live_msg_user(message_t *msg) {
	/*
	 * This function is synchronous respect to message sending.
	 */

	// try to search the resident sockets
	int rec_index = producer_get_fd_by_nickname(msg->data.hdr.receiver);

	log_trace("SOCKET INDEX IS %d", rec_index);
	if (rec_index < 0) {
		// user isn't connected.
		return false;
	}

	// lock the socket POSSIBLE ERROR DURING LOCKING
	//producer_lock_socket(rec_index);
	message_t reply;
	prepare_header(&reply.hdr);
	prepare_data(&reply.data, msg->data.hdr.receiver);

	reply.hdr.op = TXT_MESSAGE;
	strcpy(reply.hdr.sender, msg->hdr.sender);
	strcpy(reply.data.hdr.receiver, msg->data.hdr.receiver);
	reply.data.hdr.len = msg->data.hdr.len;
	reply.data.buf = msg->data.buf;

	// send message
	if (sendRequest(sockets[rec_index], &reply) <= 0) {
		log_warn("Cannot send message to user...");
		return false;
	}

	log_warn("Message sent to user!");

	return true;
}

static void worker_posttxt(int index, message_t *msg) {
	// prepare the reply
	message_hdr_t reply_hdr;
	prepare_header(&reply_hdr);

	log_debug("Message sender: %s Message Receiver: %s , Message body: %s",
			msg->hdr.sender, msg->data.hdr.receiver, msg->data.buf);

	/*
	 * sending message. first check if receiver user exists.
	 * later check if is online, if is not online, store message as unread.
	 */
	bool stop = false;
	if (strlen(msg->data.hdr.receiver) <= 0) {
		// invalid receiver
		log_warn("User cannot send message to invalid user!");
		reply_hdr.op = OP_NICK_UNKNOWN;
		stop = true;
	} else if (msg->data.buf == NULL) {
		// invalid buffer
		log_warn("User has sent NULL message (buffer = NULL)");
		reply_hdr.op = OP_FAIL;
	} else if (strlen(msg->data.buf) > (unsigned int) max_message_size) {
		log_warn("User has sent a too long message!");
		reply_hdr.op = OP_MSG_TOOLONG;
	} else {
		// message should be OK, no SQL Injection test?...
		bool exists = userman_user_exists(msg->data.hdr.receiver);

		if (exists == false) {
			log_warn("User has sent a message to unknown user...");
			reply_hdr.op = OP_NICK_UNKNOWN;
			stop = true;
		}
	}

	if (!stop) {
		// if sender == receiver, send in background
		if (strcmp(msg->hdr.sender, msg->data.hdr.receiver) == 0) {
			userman_add_message(msg->hdr.sender, msg->data.hdr.receiver,
			false, msg->data.buf, false);
			reply_hdr.op = OP_OK;

			// check if user is online
		} else if (userman_user_is_online(msg->data.hdr.receiver) == true) {
			// send message directly
			log_trace("SENDING LIVE MESSAGE");
			bool delivery = worker_send_live_msg_user(msg);

			userman_add_message(msg->hdr.sender, msg->data.hdr.receiver,
					delivery, msg->data.buf, false);

			reply_hdr.op = OP_OK;
		} else {
			// send to DB.
			userman_add_message(msg->hdr.sender, msg->data.hdr.receiver, false,
					msg->data.buf, false);
			reply_hdr.op = OP_OK;
		}
	}

	// an error is occurred, send header and stop.
	int wh_size = sendHeader(sockets[index], &reply_hdr);

	if (wh_size <= 0) {
		log_error("Cannot send reply header to user! error:%s",
				strerror(errno));
	}
}

static void worker_get_prev_msgs(int index, message_t *msg) {
	// send an ACK (i don't know why)
	message_hdr_t ack_reply;
	prepare_header(&ack_reply);

	// try to retrieve messages
	char **list = NULL;
	bool *file_list = NULL;
	int prev_msgs_n = userman_get_prev_msgs(msg->hdr.sender, &list, &file_list,
			max_hist_msgs);

	if (prev_msgs_n < 0) {
		log_fatal(
				"Cannot continue to send previous messages due to previous error.");
		ack_reply.op = OP_FAIL;
	} else if (prev_msgs_n == 0) {
		// no messages
		ack_reply.op = OP_FAIL;
	} else {
		ack_reply.op = OP_OK;
	}

	// send ack response
	if (sendHeader(sockets[index], &ack_reply) < 0) {
		log_fatal("Cannot send ack to client!");
		return;
	}

	if (ack_reply.op == OP_FAIL) {
		return;
	}

	// prepare a template of message
	message_t reply;
	prepare_data(&reply.data, msg->hdr.sender);
	prepare_header(&reply.hdr);
	size_t size_payload = (size_t) prev_msgs_n;
	reply.data.hdr.len = (unsigned int) prev_msgs_n;
	reply.data.buf = (char *) &size_payload;

	if (sendData(sockets[index], &reply.data) < 0) {
		log_fatal("Cannot send message due to previous error!");
		return;
	}

	// now send the payloads
	for (int i = 0; i < prev_msgs_n; i++) {
		// prepare the message
		reply.data.hdr.len =
				(unsigned int) (strlen(list[i]) + 1 * sizeof(char));
		reply.data.buf = list[i];
		reply.hdr.op = file_list[i] == true ? FILE_MESSAGE : TXT_MESSAGE;

		// send message
		if (sendRequest(sockets[index], &reply) < 0) {
			log_fatal("Cannot send message!");
		}

		if (file_list[i] == true) {
			// it's a file!
			log_fatal("Function not implemented.");

		}

		// free memory
		free(list[i]);
	}

	// cleanup
	free(file_list);
	free(list);
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
		worker_posttxt(index, msg);
		ret = 0;
		break;
	case POSTTXTALL_OP:
	case POSTFILE_OP:
	case GETFILE_OP:
		log_fatal("NOT IMPLEMENTED ACTIONS");
		ret = -1;
		break;
	case GETPREVMSGS_OP:
		worker_get_prev_msgs(index, msg);
		ret = 0;
		break;
	case USRLIST_OP:
		worker_user_list(index, msg, true, USERMAN_GET_ONL);
		ret = 0;
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
	// check initialization
	if (init == false) {
		log_fatal("Worker is not initialized yet.");
		/*
		 * We can implement a very simple wait_cond, if needed.
		 */
		return;
	}

	// access data pointed by body message, i.e. fd
	int *udata = (int*) message.body.bytes;
	int index = udata[1];
	int msg_type = udata[0];

	if (msg_type == SERVER_QUEUE_MESSAGE_WRITE_REQ) {
		// unlock sockets
		producer_disconnect_host(index);
		return;
	}

	message_t msg;
	int read_size;
	read_size = readMsg(sockets[index], &msg);

	if (read_size == 0) {
		/*
		 *
		 * To not leave the status of user to online status I have
		 * implemented an array that contains all nicknames with
		 * the size of max-connection
		 * each element identify the nickname connect at the socket.
		 *
		 * index  sockets  sockets_block   sockets_cln_nick
		 * 0         3         false          pippo
		 * 1         2          true          NULL (if no user)
		 * 2		...			....		  ...
		 *
		 *
		 */
		// try to recover from session
		char *nickname = NULL;
		log_debug("Try to retrieve nickname from session with socket index=%d",
				index);
		producer_get_fd_nickname(index, &nickname);
		if (nickname == NULL) {
			// cache does not contains user nickname, disconnect brutally
			producer_disconnect_host(index);
			log_debug("Nickname not found... disconnected brutally");
		} else {
			// compose the header
			strcpy(msg.hdr.sender, nickname);
			log_debug("Nickname found=%s, disconnected normally", nickname);
			free(nickname);
			worker_disconnect_user(index, &msg);
		}

		// no other operation are possible on socket.
		return;
	}

	bool c_check = check_connection(index, &msg);
	if (c_check == false) {
		// connection cannot be established.
		return;
	}

	int ra_ret = worker_action_router(index, &msg);

	// cleanup
	if (msg.data.buf != NULL) {
		free(msg.data.buf);
	}

	switch (ra_ret) {
	case -1:
		// fatal problem, disconnect
		worker_disconnect_user(index, &msg);
		break;
	case 0:
		// success, release the socket
		producer_unlock_socket(index);
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
