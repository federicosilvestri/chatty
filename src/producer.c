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

#include "producer.h"

#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <signal.h>
#include <sys/select.h>
#include <sys/time.h>
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

static int max_connections;
static const char *socket_unix_path;
static struct sockaddr_un listen_socket;
static int listen_socket_fd;
static const struct timespec select_intv = { 0, 500 };

static pthread_t producer_thread;

/**
 * Array (dynamically allocated) that contains
 * all socket file descriptors.
 */
int *sockets;

/**
 * Array (dynamically allocated) that contains a
 * boolean value to check if socket file descriptor
 * is in use or not.
 */
bool *sockets_block;

pthread_mutex_t socket_mutex;

fd_set read_fds;

static amqp_socket_t *p_socket = NULL;
static amqp_connection_state_t p_conn;
extern const char *rabmq_exchange;

/**
 * Internal function to lock the
 * socket file descriptor.
 *
 * @param index index of sockets array
 */
static void producer_lock_socket(int index) {
	pthread_mutex_lock(&socket_mutex);
	if (sockets_block[index] == true) {
		log_fatal("Socket is already locked.");
		exit(1);
	}

	sockets_block[index] = true;
	pthread_mutex_unlock(&socket_mutex);
}

void producer_unlock_socket(int index) {
	pthread_mutex_lock(&socket_mutex);
	if (sockets_block[index] == false) {
		log_fatal("Socket is already unlocked.");
		exit(1);
	}

	sockets_block[index] = false;
	pthread_mutex_unlock(&socket_mutex);
}

void producer_disconnect_host(int index) {
	int fd = sockets[index];
	close(sockets[index]);
	sockets[index] = 0;
	producer_unlock_socket(index);
	log_info("Host %d DISCONNECTED",fd);
}

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

	// check if file already exists
	if (access(socket_unix_path, F_OK) == 0) {
		log_error("Socket already exists! Is chatty running?");
		return false;
	}

	strncpy(listen_socket.sun_path, socket_unix_path, 1 + unix_path_length);

	if (config_lookup_int(&server_conf, "MaxConnections",
			&max_connections) == CONFIG_FALSE) {
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

	int opt = true;
	// Handle multiple connections
	if (setsockopt(listen_socket_fd, SOL_SOCKET, SO_REUSEADDR, (char *) &opt,
			sizeof(opt)) < 0) {
		log_error("Cannot change socket options: %s", strerror(errno));
		return false;
	}

	// Binding
	if (bind(listen_socket_fd, (const struct sockaddr*) &listen_socket,
			sizeof(listen_socket)) != 0) {
		log_error("Cannot bind socket address!: %s", strerror(errno));
		return false;
	}

	// listening
	if (listen(listen_socket_fd, max_connections) != 0) {
		log_error("Cannot listen on socket: %s", strerror(errno));
		return false;
	}

	// initialize client socket
	sockets = malloc(sizeof(int) * max_connections);
	sockets_block = malloc(sizeof(bool) * max_connections);
	for (int i = 0; i < max_connections; i++) {
		sockets[i] = 0;
		sockets_block[i] = false;
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

	if (!rabmq_init(&p_socket, &p_conn)) {
		return false;
	}

	return true;
}

/**
 * Get available index for socket.
 *
 * @return -1 if no available sockets index, else the index
 */
static inline int get_av_sock_index() {
	int av = -1;

	for (int i = 0; i < max_connections && av == -1; i++) {
		if (sockets[i] == 0) {
			av = i;
		}
	}

	return av;
}

/**
 * Initialize socket file descriptors.
 *
 * @return the maximum file descriptor
 */
static inline int run_init_socket_fds() {
	FD_ZERO(&read_fds);
	FD_SET(listen_socket_fd, &read_fds);

	int max_sd = listen_socket_fd;

	pthread_mutex_lock(&socket_mutex);

	for (int i = 0; i < max_connections; i++) {
		int sd = sockets[i];

		// If valid socket descriptor then add to read list
		if (sd > 0 && sockets_block[i] == false) {
			FD_SET(sd, &read_fds);
		}

		// Highest file descriptor number, need it for the select function
		if (sd > max_sd) {
			max_sd = sd;
		}
	}

	pthread_mutex_unlock(&socket_mutex);

	return max_sd;
}

/**
 * Manage, if it is pending, new connection.
 *
 * @return true if connection is set up, false if not
 */
static inline bool run_manage_new_conn() {
	if (FD_ISSET(listen_socket_fd, &read_fds)) {
		int new_socket = accept(listen_socket_fd, NULL, 0);

		if (new_socket < 0) {
			log_error("Error during creating new socket: %s", strerror(errno));
			return false; // do not exit, log error and try again
		}

		log_info("New connection , socket fd is %d", new_socket);

		// add new socket to array of sockets using last index
		int av = get_av_sock_index();
		sockets[av] = new_socket;

		return true;
	}

	return false;
}

/**
 * Manage the active connection.
 * If a message is in the queue, it will be sent to RabbitM (the consumer queue).
 */
static inline void run_manage_conn() {
	for (int i = 0; i < max_connections; i++) {
		int sd = sockets[i];

		if (FD_ISSET(sd, &read_fds)) {
			// pending operation
			// block the socket
			producer_lock_socket(i);
//			log_trace("Sockets %d is NOW BLOCKED", sockets[i]);

			// prepare the id
			int udata[1] = { i };
			amqp_bytes_t message_bytes;
			message_bytes.len = sizeof(int);
			message_bytes.bytes = udata;

			// put into queue
			log_trace("Publishing to queue...");
			int pub_status = amqp_basic_publish(p_conn, 1,
					amqp_cstring_bytes(rabmq_exchange), amqp_cstring_bytes(""),	// note that it should be the routing-key
					0, 0, NULL, message_bytes);

			if (pub_status != AMQP_STATUS_OK) {
				log_error("Cannot publish message to queue!");
				return;
			}

			log_info("Message successfully published! %d", pub_status);
		}
	}
}

/**
 * Clean up workspace set up by producer run thread.
 */
static inline void run_cleanup() {
// cleanup read fds
	FD_ZERO(&read_fds);

// close channel
	amqp_channel_close(p_conn, 1, AMQP_REPLY_SUCCESS);
	amqp_check_error(amqp_get_rpc_reply(p_conn), "Producer closing channel");

// closing active sockets
	for (int i = 0; i < max_connections; i++) {
		if (sockets[i] > 0) {
			close(sockets[i]);
		}
	}
}

/**
 *
 * This function is the thread function that is executed
 * by producer thread,
 *
 * @param params parameters of standard routine thread
 */
static void *producer_run(void* params) {
	log_debug("[PRODUCER THREAD] started");

// set blocking signals for select
	sigset_t s_sigset;
	sigemptyset(&s_sigset);
	sigaddset(&s_sigset, SIGINT);
	sigprocmask(SIG_BLOCK, &s_sigset, NULL);

//	// open channel to rabbit
	amqp_channel_open(p_conn, 1);
	if (amqp_check_error(amqp_get_rpc_reply(p_conn),
			"Producer opening channel") == true) {
		log_fatal("Cannot start producer");
		pthread_exit(NULL);
	}

	while (server_status() == SERVER_STATUS_RUNNING) {
		// initialize sets and get max fds
		int max_sd = run_init_socket_fds();

		/*
		 * Select execution:
		 * writefs and exceptfs are NULL
		 */
		int activity = pselect(max_sd + 1, &read_fds, NULL, NULL, &select_intv,
				&s_sigset);

		if (activity == -1) {
			// this is not a big problem
			log_debug("Producer selected failed");
			continue;
		}

		// check if there are new pending connections
		if (!run_manage_new_conn()) {
			// if no connection are discovered, manage the active
			run_manage_conn();
		}

		/*
		 *  end cycle, re-execute intialization and check,
		 *  as documented in POSIX select manual.
		 */
	}

	run_cleanup();
	log_debug("[PRODUCER THREAD] is now stopped.");

	pthread_exit(NULL);
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

void producer_wait() {
// waiting termination of main thread
	pthread_join(producer_thread, NULL);
}

void producer_destroy() {
// wait until man thread is not terminated
	log_debug("Destroying producer");

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

// Destroy RabbitMQ connection
	log_debug("Destroying RabbitMQ connection");
	rabmq_destroy(&p_conn);

// Destroying sockets
	log_debug("Destroying sockets array");
	free(sockets);
	free(sockets_block);
}
