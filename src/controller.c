/*******************************************************************************
 * SOL 2017/2018
 * Chatty
 * Federico Silvestri 559014
 * Si dichiara che il contenuto di questo file e' in ogni sua parte opera
 * originale dell'autore.
 *******************************************************************************/
/**
 * This file contains the controller component of architecture.
 * Controller is responsible for consumer and producer creation and
 * control.
 * @file controller.c
 */
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
#include <pthread.h>
#include <sys/types.h>
#include <sys/signal.h>

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

/**
 * This is the stopper thread, only for internal use.
 */
static pthread_t stopper_thread;

bool server_init() {
	/*
	 * DO NOT CHANGE ORDER OF STARTUP.
	 * IT CAN GENERATE PROBLEMS.
	 */
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
		userman_destroy();
		return false;
	}

	if (producer_init() == false) {
		userman_destroy();
		return false;
	}

	if (worker_init() == false) {
		userman_destroy();
		producer_destroy();
		return false;
	}

	if (consumer_init() == false) {
		userman_destroy();
		producer_destroy();
		consumer_destroy();
		return false;
	}

	return true;
}

bool server_start() {
	if (status == SERVER_STATUS_RUNNING) {
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
		consumer_stop();
		consumer_destroy();
		userman_destroy();
		return false;
	}

	return true;
}

static void *server_stop_run(void* arg) {
	// get the value of pointer and free it
	int *arg_pointer = (int*) arg;
	int force = *arg_pointer;
	free(arg_pointer);

	// block signals to avoid interruptions
	sigset_t s_sigset;
	sigemptyset(&s_sigset);
	sigaddset(&s_sigset, SIGINT);
	sigaddset(&s_sigset, SIGQUIT);
	sigprocmask(SIG_BLOCK, &s_sigset, NULL);

	log_trace("server_stop execution");

	// stopping producer
	producer_stop();

	// stopping consumer
	consumer_stop();

	// update status
	status = SERVER_STATUS_STOPPED;

	log_trace("server_stop executed.");

	if (force == 0) {
		// send a signal to main process to
		kill(getpid(), SIGINT);
	}

	pthread_exit(NULL);
}

bool server_stop() {
	static bool stopped = false;

	if (stopped == true) {
		log_warn("server_stop call received multiple times!");
		return false;
	}
	stopped = true;

	/*
	 * Try an asynchronous stopping.
	 */
	int *argv = malloc(sizeof(int));
	*argv = 0;
	int t_status = pthread_create(&stopper_thread, NULL, server_stop_run, argv);
	if (t_status != 0) {
		log_fatal("Cannot startup stopper thread to server");
		/*
		 * System does not accept thread submissions,
		 * we need to try a synchronous stop.
		 */
		*argv = 1;
		server_stop_run(argv);
		return true;
	}

	return true;
}

int server_status() {
	return status;
}

void server_destroy() {
	// join thread to avoid a memory leak
	pthread_join(stopper_thread, NULL);

	// producer
	producer_destroy();

	// consumer
	consumer_destroy();

	// userman
	userman_destroy();
}

/* UTILITY FUNCTIONS */

void check_mutex_lu_call(int mutex_res) {
	if (mutex_res == 0) {
		return;
	}

	log_fatal("CANNOT LOCK OR UNLOCK MUTEX. PTHREAD LIBRARY FAIL.");
	log_fatal("PTHREAD ERRNO=$d, STRERR=%s", mutex_res, strerror(mutex_res));

	exit(1);
}
