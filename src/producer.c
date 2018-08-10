/*******************************************************************************
 * SOL 2017/2018
 * Chatty
 * Federico Silvestri 559014
 * Si dichiara che il contenuto di questo file e' in ogni sua parte opera
 * originale dell'autore.
 *******************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <pthread.h>
#include <sys/un.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>

#include <amqp.h>
#include <amqp_tcp_socket.h>

#include "config.h"
#include "log.h"
#include "amqp_utils.h"
#include "producer.h"

static const char *socket_unix_path;
static struct sockaddr_un listen_socket;
static int listen_socket_fd;

static pthread_t producer_thread;

/**
 * Socket creation and initialization.
 *
 * @brief create listening socket for incoming connections
 * @return true on success, false on error
 */
static bool producer_socket_init() {
	// Preparing socket structure
	listen_socket.sun_family = AF_UNIX; // Use AF_UNIX
	if (config_lookup_string(&server_conf, "UnixPath",
			&socket_unix_path) == CONFIG_FALSE) {
		log_error(
				"Programming error, assertion failed. Cannot Get UnixPath from configuration");
		return false;
	}

	int unix_path_length = strlen(socket_unix_path);
	if (unix_path_length > 107) {
		log_error(
				"Length of 'UnixPath' parameter is greater than 107 character.");
		return false;
	}
	strncpy(listen_socket.sun_path, socket_unix_path, 1 + unix_path_length);

	// Creating socket file descriptor
	log_debug("Creating AF_UNIX socket...");
	listen_socket_fd = socket(AF_UNIX, SOCK_RAW, 0);

	if (listen_socket_fd < 0) {
		log_error("Cannot create socket: %s", strerror(errno));
		return false;
	}

	// Binding
	if (bind(listen_socket_fd, (const struct sockaddr*) &listen_socket,
			sizeof(listen_socket)) != 0) {
		log_error("Cannot bind socket address!: %s", strerror(errno));
		return false;
	}

	return true;
}

/**
 * @brief initializes all workspace for producer
 * @return true on success, false on error
 */
bool producer_init() {
	if (!producer_socket_init()) {
		return false;
	}

	return true;
}

/**
 *
 * This function is the thread function that is executed
 * by producer thread,
 *
 * @param params parameters of standard routine thread
 */
static void *producer_run(void* params) {
	//	amqp_channel_open(p_conn, 1);
	//	die_on_amqp_error(amqp_get_rpc_reply(p_conn), "Opening channel");
	//
	//	send_batch(p_conn, "test queue", p_rate_limit, p_message_count);
	//
	//	die_on_amqp_error(amqp_channel_close(p_conn, 1, AMQP_REPLY_SUCCESS),
	//			"Closing channel");

	log_debug("Producer thread is now running with success");

	sleep(10);

	log_debug("producer thread is now stopped.");

	return NULL;
}

/**
 * Start the producer thread.
 *
 * @return true on success, false on error
 */
bool producer_start() {
	// starting thread of producer
	int t_status = pthread_create(&producer_thread, NULL, producer_run, NULL);

	if (t_status != 0) {
		log_error("Cannot startup producer", strerror(errno));
		return false;
	}

	return true;
}

void producer_destroy() {
	// close socket to rabbit
	log_debug("Closing RabbitMQ connection");

//	amqp_check_error(amqp_connection_close(p_conn, AMQP_REPLY_SUCCESS),
//			"Cannot close connection to RabbitM");

	// try to destroy (also if it isn't closed)

//	log_debug("Destroying RabbitMQ connection");
//	int c_destroy = amqp_destroy_connection(p_conn);
//
//	if (c_destroy < 0) {
//		log_error("Cannot destroy connection to RabbitMQ, %s",
//				amqp_error_string2(c_destroy));
//	}

	log_debug("Closing listening socket");
	// closing socket connection
	if (close(listen_socket_fd) < 0) {
		log_error("Cannot close listen socket file descriptor");
	}

	// deleting UNIX socket
	if (unlink(socket_unix_path) != 0) {
		log_error("Cannot delete Unix socket file");
	}
}
