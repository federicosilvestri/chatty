/*******************************************************************************
 * SOL 2017/2018
 * Chatty
 * Federico Silvestri 559014
 * Si dichiara che il contenuto di questo file e' in ogni sua parte opera
 * originale dell'autore.
 *******************************************************************************/
/**
 * This file contains the consumer component of the architecture.
 * The consumer dequeues messages from RabbitMQ and processes it.
 * @brief File that contains the functions of Consumer component.
 * @file consumer.c
 */

/**
 * C POSIX source definition.
 */
#define _POSIX_C_SOURCE 200809L

#include "consumer.h"

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <libconfig.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/signal.h>
#include <amqp.h>
#include <amqp_tcp_socket.h>

#include "log.h"
#include "amqp_utils.h"
#include "controller.h"
#include "worker.h"

/**
 * External configuration struct by libconfig.
 */
extern config_t server_conf;

/**
 * System status
 */
static int status = SERVER_STATUS_STOPPED;

/**
 * Mutex for system status
 */
static pthread_mutex_t status_mutex;

/**
 * Array of consumer threads
 */
static pthread_t *consumer_threads;

/**
 * Variable to config how many consumer threads must be started.
 */
static int consumer_threads_n;

/**
 * Array that contains consumer thread ids.
 */
static int *consumer_threads_ids;

static int consumer_get_status() {
	int ret;

	check_mutex_lu_call(pthread_mutex_lock(&status_mutex));
	ret = status;
	check_mutex_lu_call(pthread_mutex_unlock(&status_mutex));

	return ret;
}

static void consumer_set_status(int new_status) {
	check_mutex_lu_call(pthread_mutex_lock(&status_mutex));
	status = new_status;
	check_mutex_lu_call(pthread_mutex_unlock(&status_mutex));
}

bool consumer_init() {
	bool initialized = false;

	if (initialized == true) {
		log_fatal("Consumer is already initialized!");
		return false;
	}

	pthread_mutex_init(&status_mutex, NULL);

	// set status (for security
	status = SERVER_STATUS_STOPPED;

	// get configuration
	if (config_lookup_int(&server_conf, "ThreadsInPool",
			&consumer_threads_n) == CONFIG_FALSE) {
		log_fatal(
				"Programming error, cannot get ThreadsInPool parameter from configuration");
		return false;
	}

	if (consumer_threads_n <= 0) {
		log_error("Cannot startup %d threads, bad parameter",
				consumer_threads_n);
		return false;
	}

	consumer_threads = malloc(
			sizeof(pthread_t) * ((long unsigned int) consumer_threads_n));
	consumer_threads_ids = malloc(
			sizeof(unsigned int) * ((long unsigned int) consumer_threads_n));

	for (int i = 0; i < consumer_threads_n; i++) {
		consumer_threads_ids[i] = i;
	}

	initialized = true;
	return true;
}

static bool consumer_run_exception(amqp_connection_state_t conn,
		amqp_rpc_reply_t ret) {
	if (ret.reply_type == AMQP_RESPONSE_LIBRARY_EXCEPTION
			&& ret.library_error == AMQP_STATUS_UNEXPECTED_STATE) {
		log_warn(
				"[CONSUMER THREAD] Exception during message consuming... reply_type=%d",
				ret.reply_type);

		amqp_frame_t frame;

		if (amqp_simple_wait_frame(conn, &frame) != AMQP_STATUS_OK) {
			log_fatal("Simple wait frame failed");
			return true;
		}

		if (frame.frame_type == AMQP_FRAME_METHOD) {
			switch (frame.payload.method.id) {
			case AMQP_BASIC_ACK_METHOD:
				log_fatal("ACK not implemented.");
				return true;
			case AMQP_BASIC_RETURN_METHOD:
				log_fatal("MANDATORY RETURN is not implemented.");
				return true;
			case AMQP_CONNECTION_CLOSE_METHOD:
				log_fatal("Unexpected connection closing. Cleaning up...");
				return true;
			case AMQP_CHANNEL_CLOSE_METHOD:
				log_fatal("Unexpected channel closing. Cleaning up...");
				return true;
				break;
			default:
				log_fatal("An unexpected method was received %u",
						frame.payload.method.id);
				return true;
			}
		}
	}

	return false;
}

static void consumer_run_wait(amqp_connection_state_t conn, int tid) {
	while (consumer_get_status() == SERVER_STATUS_RUNNING) {
		amqp_rpc_reply_t ret;
		amqp_envelope_t envelope;
		struct timeval timeout = { 1, 0 };

		amqp_maybe_release_buffers(conn);
		ret = amqp_consume_message(conn, &envelope, &timeout, 0);

		// Exception management
		if (ret.reply_type != AMQP_RESPONSE_NORMAL) {
			if (consumer_run_exception(conn, ret)) {
				log_fatal("Application PANIC: you must restart");
				break;
			}
		} else {
			log_trace("[CONSUMER THREAD %d] processing message in the queue",
					tid);

			// execute the worker
			worker_run(envelope.message);

			amqp_destroy_envelope(&envelope);
		}
	}
}

static void *consumer_run(void *index_addr) {
	// thread identification
	const int id = *((int *) index_addr);

	// RabbitMQ connection
	amqp_socket_t *c_socket = NULL;
	amqp_connection_state_t c_conn;

	// initialize connection to RabbitMQ
	if (rabmq_init(&c_socket, &c_conn) == false) {
		log_error("Cannot connect to RabbitMQ");
		rabmq_destroy(&c_conn);
		pthread_exit(NULL);
	}

	log_debug("[CONSUMER THREAD %d] started", id);

	// open rabbit channel
	amqp_channel_open(c_conn, 1);
	if (amqp_check_error(amqp_get_rpc_reply(c_conn),
			"Consumer opening channel") == true) {
		log_fatal("Cannot start consumer");
		pthread_exit(NULL);
	}

	log_trace("Set the basic consume queue...");
	amqp_basic_consume(c_conn, 1, amqp_cstring_bytes(RABBIT_QUEUE_NAME),
			amqp_empty_bytes, 0, 1, 0, amqp_empty_table);

	amqp_check_error(amqp_get_rpc_reply(c_conn), "Setting consuming queue");

	consumer_run_wait(c_conn, id);

	// close channel
	amqp_channel_close(c_conn, 1, AMQP_REPLY_SUCCESS);
	amqp_check_error(amqp_get_rpc_reply(c_conn), "Consumer closing channel");

	// destroying connection
	rabmq_destroy(&c_conn);

	log_debug("[CONSUMER THREAD %d] finished", id);
	pthread_exit(NULL);
}

bool consumer_start() {
	// check if server is already started
	if (consumer_get_status() != SERVER_STATUS_STOPPED) {
		log_fatal("Consumer is already started!");
		return false;
	}

	// no concurrency, set status
	status = SERVER_STATUS_RUNNING;

	for (int i = 0; i < consumer_threads_n; i++) {

		int p_stat = pthread_create(&consumer_threads[i], NULL, consumer_run,
				&consumer_threads_ids[i]);

		if (p_stat != 0) {
			log_fatal("Cannot create consumer pthread %s", strerror(errno));
			return false;
		}
	}

	return true;
}

void consumer_stop() {
	// set the stop status
	consumer_set_status(SERVER_STATUS_STOPPED);

	for (int i = 0; i < consumer_threads_n; i++) {
		log_debug("Waiting for consumer thread %d", i);
		pthread_join(consumer_threads[i], NULL);
		log_debug("Consumer thread %d done", i);
	}
}

void consumer_destroy() {
	if (consumer_get_status() != SERVER_STATUS_STOPPED) {
		log_fatal("Cannot destroy consumer when it's in running status!");
		return;
	}

	log_trace("executing destroying producer...");

	pthread_mutex_destroy(&status_mutex);
	free(consumer_threads);
	free(consumer_threads_ids);

	log_trace("executed destroying producer");
}
