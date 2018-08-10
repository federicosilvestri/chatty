/*******************************************************************************
 * SOL 2017/2018
 * Chatty
 * Federico Silvestri 559014
 * Si dichiara che il contenuto di questo file e' in ogni sua parte opera
 * originale dell'autore.
 *******************************************************************************/
#include "producer.h"

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

#include "controller.h"
#include "config.h"
#include "log.h"
#include "amqp_utils.h"

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
				"Programming error, assertion failed. Cannot get UnixPath from configuration");
		return false;
	}

	int unix_path_length = strlen(socket_unix_path);
	if (unix_path_length > 107) {
		log_error(
				"Length of 'UnixPath' parameter is greater than 107 character.");
		return false;
	}
	strncpy(listen_socket.sun_path, socket_unix_path, 1 + unix_path_length);

	int maxConnections;
	if (config_lookup_int(&server_conf, "MaxConnections",
			&maxConnections) == CONFIG_FALSE) {
		log_error(
				"Programming error, assertion failed. Cannot get MaxConnections from configuration)");
		return false;
	}

	// Creating socket file descriptor
	log_debug("Creating AF_UNIX socket...");
	listen_socket_fd = socket(AF_UNIX, SOCK_STREAM, 0);

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

	// listening
	if (listen(listen_socket_fd, maxConnections) != 0) {
		log_error("Cannot listen on socket: %s", strerror(errno));
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
	log_debug("Producer is alive");

	while (server_status() == SERVER_STATUS_RUNNING) {
		log_warn("Non so che fare, sono in status di running...");
		sleep(3);
	}

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
	int t_status = pthread_create(&producer_thread, NULL, producer_run,
	NULL);

	if (t_status != 0) {
		log_error("Cannot startup producer", strerror(errno));
		return false;
	}

	log_debug("Producer started successfully");

	return true;
}

void producer_join() {
	// waiting termination of main thread
	pthread_join(producer_thread, NULL);
}

void producer_destroy() {
	log_debug("Closing listening socket");
	// closing socket connection
	if (close(listen_socket_fd) < 0) {
		log_error("Cannot close listen socket file descriptor");
	}

	log_debug("Deleting socket file");
	// deleting UNIX socket
	if (unlink(socket_unix_path) != 0) {
		log_error("Cannot delete Unix socket file");
	}
}
