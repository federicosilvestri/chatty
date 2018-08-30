/*******************************************************************************
 * SOL 2017/2018
 * Chatty
 * Federico Silvestri 559014
 * Si dichiara che il contenuto di questo file e' in ogni sua parte opera
 * originale dell'autore.
 *******************************************************************************/

/**
 * This file is not a signal handler but it's a signal manager.
 * It doesn't set a specific sigaction for a specific signal,
 * it registers all signals to be managed by program, using
 * the `signal_manager_wait_signal` function.
 *
 * @brief this file contains the functions to manage signals.
 * @file signal_manager.c signal management header file.
 */

/**
 * Define Posix Source
 */
#define _POSIX_C_SOURCE 200809L

#include "signal_manager.h"

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <pthread.h>
#include <errno.h>
#include <sys/signal.h>

#include "log.h"

/**
 * Signals to register.
 */
static const int signals[] = { SIGPIPE, SIGINT, SIGUSR1, SIGQUIT, SIGTERM };

/**
 * Registered signal to register to system
 */
static sigset_t signal_set;

/**
 * Old signal set
 */
static sigset_t old_signal_set;

bool signal_manager_register() {
	static bool init = false;

	if (init) {
		log_error("Signal handler is already initialized");
		return false;
	}

	// initialize set
	if (sigemptyset(&signal_set) == -1) {
		log_fatal("Cannot initialize signal set.");
		return false;
	}

	// adding signals
	for (unsigned int i = 0; i < sizeof(signals) / sizeof(const int); i++) {
		if (sigaddset(&signal_set, signals[i]) == -1) {
			log_fatal("Cannot add signal to signals set");
			return false;
		}
	}

	// setting block signal
	sigprocmask(SIG_BLOCK, &signal_set, &old_signal_set);

	init = true;
	return true;
}

int signal_manager_wait() {
	int signal = -1;

	if (sigwait(&signal_set, &signal) == -1) {
		log_error("Cannot wait signals...");
	}

	return signal;
}

void signal_manager_destroy() {
	// free unused variables
	log_debug("Destroying signal manager...");
}
