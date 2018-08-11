/*******************************************************************************
 * SOL 2017/2018
 * Chatty
 * Federico Silvestri 559014
 * Si dichiara che il contenuto di questo file e' in ogni sua parte opera
 * originale dell'autore.
 *******************************************************************************/

/**
 * @file signal_handler.h signal management header file.
 * @brief it manages all signals triggered by system.
 */

/**
 * Define Posix Source
 */
#define _POSIX_C_SOURCE 200809L

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
#include "controller.h"

/**
 * Registered signal to register to system
 */
const int sig_handl_signals[] = { SIGPIPE, SIGINT };

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

	// Setting SIG_PIPE
	sig_handl_acts[SIG_HANDL_SIGPIPE].sa_handler = signal_handler_pipe; // it can be replaced with SIG_IGN
	sig_handl_acts[SIG_HANDL_SIGPIPE].sa_flags = SA_SIGINFO;

	// setting up SIG_INT
	sig_handl_acts[SIG_HANDL_SIGINT].sa_handler = signal_handler_int;

	sig_handl_acts[3].sa_handler = signal_handler_int;

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

void signal_handler_int() {
	switch (server_status()) {
	case SERVER_STATUS_RUNNING:
		// stopping server
		server_stop();
		break;
	case SERVER_STATUS_STOPPED:
		log_warn("Server is already stopped.");
		break;
	case SERVER_STATUS_STOPPING:
		log_info("One Moment! Stop command already received!");
		break;
	default:
		log_debug("Received SIGINT signal, unknown server status.. %s",
				"use \"pkill -9 chatty\" in case of emergency");
	}
}

void signal_handler_term() {
	log_debug("Received SIGTERM signal, managing it...");
}

void signal_handler_usr1() {
	log_debug("Received SIGUSR1 signal, managing it...");

}
