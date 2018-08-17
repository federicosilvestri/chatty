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

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>

#include "worker.h"
#include "producer.h"
#include "log.h"
#include "controller.h"

extern int *sockets;
extern bool *sockets_block;

void worker_run(amqp_message_t message) {
	// access data pointed by body message, i.e. fd
	int *udata = (int*) message.body.bytes;
	int index = udata[1];

	// read bytes from file descriptor
	int read_size;
	char *buffer = NULL;

	buffer = calloc(sizeof(char), 1024);
	read_size = read(sockets[index], buffer, 1024);
	// release sockets block
	sockets_block[index] = false;
	log_trace("SOCKET %d is now UNLOCKED", sockets[index]);

	if (read_size == 0) {
		log_error("Bad protocol for client!");
	}
	log_info("Buffer contains: %s", buffer);
	free(buffer);
}
