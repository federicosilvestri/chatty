/*******************************************************************************
 * SOL 2017/2018
 * Chatty
 * Federico Silvestri 559014
 * Si dichiara che il contenuto di questo file e' in ogni sua parte opera
 * originale dell'autore.
 *******************************************************************************/
/**
 * This file contains functions related to producer component.
 * Producer is the component that produces requests to be sent
 * to queue.
 * @file producer.c
 */

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

#include "message.h"
#include "stats.h"

#include "controller.h"
#include "config.h"
#include "log.h"
#include "amqp_utils.h"

/**
 * Max connection that producer can handle
 */
int producer_max_connections;

/**
 * The place where create the socket file
 */
static const char *socket_unix_path;

/**
 * POSIX struct for listen socket
 */
static struct sockaddr_un listen_socket;

/**
 * Listen file descriptor
 */
static int listen_socket_fd;

/**
 * Time interval for select function
 */
static const struct timespec select_intv = { 0, 500 };

/**
 * Internal producer threads
 */
static pthread_t producer_thread;

/**
 * System status
 */
static int status = SERVER_STATUS_STOPPED;

/**
 * Mutex for system status
 */
static pthread_mutex_t status_mutex;

/**
 * Array (dynamically allocated) that contains
 * all socket file descriptors
 */
int *sockets;

/**
 * Array (dynamically allocated) that contains a
 * boolean value to check if socket file descriptor
 * is in use or not
 */
static bool *sockets_block;

/**
 * Array (partially dynamically allocated) that contains a
 * string value of user that is connected to the current socket
 */
static char **sockets_cu_nick;

/**
 * Mutex for giving an exclusive access
 * to array of sockets
 */
pthread_mutex_t socket_mutex;

/**
 * Set of read file descriptor
 */
fd_set read_fds;

/**
 * Set of write file descriptors
 */
fd_set write_fds;

/**
 * Socket structure for RabbitMQ connection
 */
static amqp_socket_t *p_socket = NULL;

/**
 * State of RabbitMQ connection
 */
static amqp_connection_state_t p_conn;

/**
 * The name of the exchange for RabbitMQ
 */
extern const char *rabmq_exchange;

static int producer_get_status() {
	int ret;

	check_mutex_lu_call(pthread_mutex_lock(&status_mutex));
	ret = status;
	check_mutex_lu_call(pthread_mutex_unlock(&status_mutex));

	return ret;
}

static void producer_set_status(int new_status) {
	check_mutex_lu_call(pthread_mutex_lock(&status_mutex));
	status = new_status;
	check_mutex_lu_call(pthread_mutex_unlock(&status_mutex));
}

static bool producer_socket_init() {
	// Preparing socket structure
	listen_socket.sun_family = AF_UNIX; // Use AF_UNIX
	if (config_lookup_string(&server_conf, "UnixPath",
			&socket_unix_path) == CONFIG_FALSE) {
		log_error(
				"Programming error, assertion failed. Cannot get UnixPath from configuration");
		return false;
	}

	size_t unix_path_length = strlen(socket_unix_path);
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
			&producer_max_connections) == CONFIG_FALSE) {
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
	if (listen(listen_socket_fd, producer_max_connections) != 0) {
		log_error("Cannot listen on socket: %s", strerror(errno));
		return false;
	}

	// initialize client socket
	sockets = malloc(
			sizeof(int) * ((long unsigned int) producer_max_connections));
	sockets_block = malloc(
			sizeof(bool) * ((long unsigned int) producer_max_connections));
	sockets_cu_nick = malloc(
			sizeof(char*) * ((long unsigned int) producer_max_connections));

	for (int i = 0; i < producer_max_connections; i++) {
		sockets[i] = 0;
		sockets_block[i] = false;
		sockets_cu_nick[i] = calloc(sizeof(char),
				sizeof(char) * (MAX_NAME_LENGTH + 1));
		sockets_cu_nick[i][0] = '\0';
	}

	return true;
}

/**
 * @brief initializes all workspace for producer
 * @return true on success, false on error
 */
bool producer_init() {
	static bool initialized = false;

	if (initialized == true) {
		log_fatal("Producer is already initialized!");
		return false;
	}

	// initialize mutex
	pthread_mutex_init(&status_mutex, NULL);

	if (!producer_socket_init()) {
		return false;
	}

	if (!rabmq_init(&p_socket, &p_conn)) {
		return false;
	}

	status = SERVER_STATUS_STOPPED;
	initialized = true;

	return true;
}

void producer_lock_socket(int index) {
	check_mutex_lu_call(pthread_mutex_lock(&socket_mutex));
	if (sockets_block[index] == true) {
		log_fatal("Socket is already locked.");
		exit(1);
	}

	sockets_block[index] = true;
	check_mutex_lu_call(pthread_mutex_unlock(&socket_mutex));
	log_trace("SOCKET index %d LOCKED", index);
}

void producer_unlock_socket(int index) {
	check_mutex_lu_call(pthread_mutex_lock(&socket_mutex));
	if (sockets_block[index] == false) {
		log_fatal("Socket is already unlocked.");
		exit(1);
	}

	sockets_block[index] = false;
	check_mutex_lu_call(pthread_mutex_unlock(&socket_mutex));
	log_trace("SOCKET index %d UNLOCKED", index);
}

void producer_disconnect_host(int index) {
	int fd = sockets[index];
	close(sockets[index]);
	sockets[index] = 0;
	producer_set_fd_nickname(index, NULL);
	producer_unlock_socket(index);
	log_info("Host %d DISCONNECTED", fd);
}

void producer_set_fd_nickname(int index, char *nickname) {
	if (index < 0 || index > producer_max_connections) {
		log_fatal("Programming error, cannot set an invalid index of socket");
		return;
	}

	check_mutex_lu_call(pthread_mutex_lock(&socket_mutex));
	if (nickname == NULL) {
		sockets_cu_nick[index][0] = '\0';
	} else {
		strcpy(sockets_cu_nick[index], nickname);
	}
	check_mutex_lu_call(pthread_mutex_unlock(&socket_mutex));
}

void producer_get_fd_nickname(int index, char **nickname) {
	if (nickname == NULL) {
		log_fatal("You cannot pass a null pointer!");
		return;
	}

	check_mutex_lu_call(pthread_mutex_lock(&socket_mutex));
	if (sockets_cu_nick[index][0] != '\0') {
		*nickname = calloc(sizeof(char), sizeof(char) * (MAX_NAME_LENGTH + 1));
		strcpy(*nickname, sockets_cu_nick[index]);
	}
	check_mutex_lu_call(pthread_mutex_unlock(&socket_mutex));
}

int producer_get_fd_by_nickname(char* nickname) {
	if (nickname == NULL) {
		return -1;
	}

	check_mutex_lu_call(pthread_mutex_lock(&socket_mutex));
	int index = -1;
	for (int i = 0; i < producer_max_connections && index == -1; i++) {
		if (strcmp(nickname, sockets_cu_nick[i]) == 0) {
			index = i;
		}
	}
	check_mutex_lu_call(pthread_mutex_unlock(&socket_mutex));

	return index;
}

int producer_get_fds_n_by_nickname(char *nickname) {
	if (nickname == NULL) {
		return -1;
	}

	check_mutex_lu_call(pthread_mutex_lock(&socket_mutex));
	int fd_n = 0;
	for (int i = 0; i < producer_max_connections; i++) {
		if (strcmp(nickname, sockets_cu_nick[i]) == 0) {
			fd_n += 1;
		}
	}
	check_mutex_lu_call(pthread_mutex_unlock(&socket_mutex));

	return fd_n;
}

int producer_get_fds_n() {
	check_mutex_lu_call(pthread_mutex_lock(&socket_mutex));
	int fd_n = 0;
	for (int i = 0; i < producer_max_connections; i++) {
		if (strlen(sockets_cu_nick[i]) > 0) {
			fd_n += 1;
		}
	}
	check_mutex_lu_call(pthread_mutex_unlock(&socket_mutex));

	return fd_n;
}

// SELECT FUNCTION (you cannot call it outside, due to mutex lock)

static inline int get_av_sock_index() {
	int av = -1;

	for (int i = 0; i < producer_max_connections && av == -1; i++) {
		if (sockets[i] == 0) {
			av = i;
		}
	}

	return av;
}

static inline int run_init_socket_fds() {
	FD_ZERO(&read_fds);
	FD_ZERO(&write_fds);

	FD_SET(listen_socket_fd, &read_fds);

	int max_sd = listen_socket_fd;

	for (int i = 0; i < producer_max_connections; i++) {
		int sd = sockets[i];

		// If valid socket descriptor then add to read list
		if (sd > 0 && sockets_block[i] == false) {
			FD_SET(sd, &read_fds);
			FD_SET(sd, &write_fds);
		}

		// Highest file descriptor number, need it for the select function
		if (sd > max_sd) {
			max_sd = sd;
		}
	}

	return max_sd;
}

static inline bool run_manage_new_conn() {
	if (FD_ISSET(listen_socket_fd, &read_fds)) {
		int new_socket = accept(listen_socket_fd, NULL, NULL);

		if (new_socket < 0) {
			log_error("Error during creating new socket: %s", strerror(errno));
			return false; // do not exit, log error and try again
		}

		log_info("New connection, socket fd is %d", new_socket);

		// ignore standard I/O sockets
		if (new_socket < 3) {
			log_fatal("BAD SOCKET.");
			exit(1);
		}

		// add new socket to array of sockets using last index
		int av = get_av_sock_index();

		if (av != -1) {
			sockets[av] = new_socket;
		} else {
			log_warn(
					"Server overload, rejecting connections...(INCREASE CONNECTION NUMBER)");
			close(new_socket);
		}

		return true;
	}

	return false;
}

static inline void run_manage_conn() {
	// locking the entire array

	for (int i = 0; i < producer_max_connections; i++) {
		int sd = sockets[i];
		if (sockets[i] <= 0 || sockets_block[i] == true) {
			continue;
		}

		// check if we need to publish something or not
		bool publish = false;
		// message to send
		int udata[2];

		if (FD_ISSET(sd, &read_fds)) {
			// pending operation
			udata[0] = SERVER_QUEUE_MESSAGE_CMD_REQ;
			publish = true;
		} else if (FD_ISSET(sd, &write_fds)) {
			/*
			 *  Data required.
			 *  This is tipically used  to detect if someone is
			 *  waiting messages.
			 */
			udata[0] = SERVER_QUEUE_MESSAGE_WRITE_REQ;
			publish = true;
		}

		if (publish == true) {
			// lock the socket
			sockets_block[i] = true;

			// set the socket to process
			udata[1] = i;

			// message to send to RabbitMQ
			amqp_bytes_t message_bytes;
			message_bytes.len = sizeof(int) * 2;
			message_bytes.bytes = udata;

			// push into queue
			log_trace("Publishing socket %d to the queue...", sockets[i]);
			int pub_status = amqp_basic_publish(p_conn, 1,
					amqp_cstring_bytes(rabmq_exchange), amqp_cstring_bytes(""),	// note that it should be the routing-key
					0, 0, NULL, message_bytes);

			if (pub_status != AMQP_STATUS_OK) {
				log_error("Cannot publish message to queue!");
				return;
			}

			log_trace("Socket %d successfully published! %d", sockets[i],
					pub_status);
		}
	}
}

static inline void run_cleanup() {
	// cleanup read fds
	FD_ZERO(&read_fds);
	FD_ZERO(&write_fds);

	// close channel
	amqp_channel_close(p_conn, 1, AMQP_REPLY_SUCCESS);
	amqp_check_error(amqp_get_rpc_reply(p_conn), "Producer closing channel");

	/*
	 * DO NOT CLOSE
	 * the opened socket.
	 * leave the close operation to destroy.
	 */
}

static void *producer_run() {
	log_debug("[PRODUCER THREAD] started");

	// set blocking signals for select to avoid bad connections
	sigset_t s_sigset;
	sigemptyset(&s_sigset);
	sigaddset(&s_sigset, SIGINT);
	sigaddset(&s_sigset, SIGQUIT);
	sigaddset(&s_sigset, SIGTERM);
	sigaddset(&s_sigset, SIGUSR1);
	sigaddset(&s_sigset, SIGUSR2);
	sigprocmask(SIG_BLOCK, &s_sigset, NULL);

	// open channel to rabbit
	amqp_channel_open(p_conn, 1);
	if (amqp_check_error(amqp_get_rpc_reply(p_conn),
			"Producer opening channel") == true) {
		log_fatal("Cannot start producer");
		pthread_exit(NULL);
	}

	while (producer_get_status() == SERVER_STATUS_RUNNING) {
		// lock
		check_mutex_lu_call(pthread_mutex_lock(&socket_mutex));

		// initialize sets and get max fds
		int max_sd = run_init_socket_fds();

		// unlock to allow other threads to perform operations on socket management
		check_mutex_lu_call(pthread_mutex_unlock(&socket_mutex));

		/*
		 * Select execution:
		 * writefs and exceptfs are NULL
		 */
		int activity = pselect(max_sd + 1, &read_fds, &write_fds, NULL,
				&select_intv, &s_sigset);

		if (activity == -1) {
			// this is not a big problem
			log_debug("Producer selected failed");
			continue;
		}

		// lock for sockets management
		check_mutex_lu_call(pthread_mutex_lock(&socket_mutex));
		// check if there are new pending connections
		if (!run_manage_new_conn()) {
			// if no connection are discovered, manage the active
			run_manage_conn();
		}

		check_mutex_lu_call(pthread_mutex_unlock(&socket_mutex));

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
	// get current status
	if (producer_get_status() != SERVER_STATUS_STOPPED) {
		log_fatal("Producer is already started!");
		return false;
	}

	// starting thread of producer, no concurrency, set the status without locks
	status = SERVER_STATUS_RUNNING;

	int t_status = pthread_create(&producer_thread, NULL, producer_run,
	NULL);

	if (t_status != 0) {
		log_error("Cannot startup producer", strerror(errno));
		return false;
	}

	log_debug("Producer started successfully");

	return true;
}

void producer_stop() {
	producer_set_status(SERVER_STATUS_STOPPED);
	// waiting termination of main thread
	pthread_join(producer_thread, NULL);
}

void producer_destroy() {
	log_debug("Destroying producer");

	// closing active sockets
	for (int i = 0; i < producer_max_connections; i++) {
		if (sockets[i] > 0) {
			close(sockets[i]);
		}
	}

	pthread_mutex_destroy(&status_mutex);

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
	// free the sockets nicks matrix
	for (int i = 0; i < producer_max_connections; i++) {
		free(sockets_cu_nick[i]);
	}
	free(sockets_cu_nick);
}
