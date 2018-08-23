/*******************************************************************************
 * SOL 2017/2018
 * Chatty
 * Federico Silvestri 559014
 * Si dichiara che il contenuto di questo file e' in ogni sua parte opera
 * originale dell'autore.
 *******************************************************************************/
/**
 * C POSIX source definition.
 */
#define _POSIX_C_SOURCE 200809L

#include "controller.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>

#include <amqp.h>
#include <amqp_tcp_socket.h>

#include "config.h"
#include "log.h"
#include "amqp_utils.h"
#include "producer.h"
#include "consumer.h"
#include "userman.h"
#include "worker.h"

/**
 * This is the internal status variable.
 * It must not be affected by concurrency race.
 */
static int status = SERVER_STATUS_STOPPED;

bool server_init() {
	if (status != SERVER_STATUS_STOPPED) {
		return false;
	}

	if (rabmq_init_params() == false) {
		return false;
	}

	if (rabmq_declare_init() == false) {
		return false;
	}

	if (userman_init() == false) {
		return false;
	}

	if (worker_init() == false) {
		return false;
	}

	if (producer_init() == false) {
		return false;
	}

	if (consumer_init() == false) {
		return false;
	}

	return true;
}

bool server_start() {
	if (status == SERVER_STATUS_STOPPING || status == SERVER_STATUS_RUNNING) {
		log_error("server_start call received multiple times!");
		return false;
	}

	// update status
	status = SERVER_STATUS_RUNNING;

	if (consumer_start() == false) {
		producer_destroy();
		consumer_destroy();
		userman_destroy();
		return false;
	}

	if (producer_start() == false) {
		producer_destroy();
		server_stop();
		consumer_wait();
		consumer_destroy();
		userman_destroy();
		return false;
	}

	return true;
}

bool server_stop() {
	log_trace("server_stop execution");

	if (status == SERVER_STATUS_STOPPING) {
		log_warn("server_stop call received multiple times!");
		return false;
	}

	// update status
	status = SERVER_STATUS_STOPPED;

	log_trace("server_stop executed.");

	return true;
}

int server_status() {
	return status;
}

void server_wait() {
	// wait termination of consumer thread
	producer_wait();
	consumer_wait();
}

void server_destroy() {
	// producer
	producer_destroy();

	// consumer
	consumer_destroy();

	// userman
	userman_destroy();
}
