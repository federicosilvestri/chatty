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

#include "producer.h"
#include "log.h"
#include "controller.h"

extern int *sockets;
extern bool *sockets_block;

static void disconnect_host(int index) {
	int fd = sockets[index];

	close(sockets[index]);
	sockets[index] = 0;
	log_info("Host disconnected");

	// release sockets block
	sockets_block[index] = false;
	log_trace("SOCKET %d is now UNLOCKED", fd);
}

static short int check_header(message_hdr_t hdr) {
	if (hdr.sender == NULL || strlen(hdr.sender) == 0) {
		return 1;
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
		disconnect_host(index);
		return;
	}

	if (check_header(msg.hdr) == 1) {
		log_warn("Someone has sent anonymous message, rejecting");
		disconnect_host(index);
	}

	// read body of message
	log_error("Message header contains sender=%s", msg.hdr.sender);

}
