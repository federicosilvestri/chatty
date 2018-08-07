/*
 * signal_handler.c
 *
 *  Created on: Aug 4, 2018
 *      Author: federicosilvestri
 */

/**
 * @file signal_handler.h signal management header file.
 * @brief it manages all signals triggered by system.
 */
#include "signal_handler.h"

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <pthread.h>
#include <stdbool.h>
#include <errno.h>

#include "log.h"

/**
 * Registered signal to register to system
 */
const int sig_handl_signals[] = { SIGPIPE };

/**
 * Actions to perform when signal is triggered.
 */
struct sigaction sig_handl_acts[SIG_HANDL_N];

void signal_handler_init() {
	static bool init = false;

	if (init) {
		log_error("Signal handler is already initialized");
		return;
	}

	for (int i = 0; i < SIG_HANDL_N; i++) {
		// set memory for data
		memset(&sig_handl_acts[i], 0, sizeof(sig_handl_acts[i]));

	}

	// setting up SIG_PIPE
	sig_handl_acts[SIG_HANDL_SIGPIPE].sa_handler = signal_handler_pipe; // it can be replaced with SIG_IGN
	sig_handl_acts[SIG_HANDL_SIGPIPE].sa_flags = SA_SIGINFO;
	sig_handl_acts[SIG_HANDL_SIGTERM].sa_handler = signal_handler_term;
	sig_handl_acts[SIG_HANDL_SIGUSR1].sa_handler = signal_handler_usr1;

	init = true;
}


bool signal_handler_register() {
	log_debug("Registering signal handler to system");

	unsigned int sig_err = -1;
	for (int i = 0; i < SIG_HANDL_N && sig_err == -1; i++) {
		// registering
		if (sigaction(sig_handl_signals[i], &sig_handl_acts[i], NULL) == -1) {
			sig_err = i;
		}

		log_debug("SIG %d registered", sig_handl_signals[i]);
	}

	if (sig_err > -1) {
		log_error("Cannot register signal handler %d, due to: %s", sig_err,
				strerror(errno));
		return false;
	}

	log_debug("Signal handlers registered");

	return true;
}

void signal_handler_pipe() {
	log_debug("Received SIGPIPE signal, ignoring it...");
}

void signal_handler_term() {
	log_debug("Received SIGTERM signal, managing it...");
}

void signal_handler_usr1() {
	log_debug("Received SIGUSR1 signal, managing it...");

}
