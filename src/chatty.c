/*******************************************************************************
 * SOL 2017/2018
 * Chatty
 * Federico Silvestri 559014
 * Si dichiara che il contenuto di questo file e' in ogni sua parte opera
 * originale dell'autore.
 *******************************************************************************/

/**
 * @file chatty.c
 * @brief File principale del server chatterbox
 */

/**
 * Define Posix Source
 */
#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>

#include <errno.h>
#include <libconfig.h>
#include <signal.h>

#include "log.h"
#include "stats.h"
#include "config.h"
#include "controller.h"
#include "signal_manager.h"

#include "chatty.h"

/**
 * Variable to register statistics.
 */
struct statistics chattyStats = { 0, 0, 0, 0, 0, 0, 0 };

/**
 * Usage function.
 * @param progname name of program
 */
static void usage(const char *progname) {
	fprintf(stderr, "Il server va lanciato con il seguente comando:\n");
	fprintf(stderr, "  %s -f <configuration-file>\n", progname);
}

/**
 * Main function that is called from system.
 * @param argc argument count
 * @param argv array of string that contains arguments
 * @return program exit code.
 */
int main(int argc, char *argv[]) {
	log_init();

	if (check_arguments(argc, argv) == false) {
		return 1;
	}

	if (config_parse(argv[2]) == false) {
		return 1;
	}

	// try to register signals
	if (signal_manager_register() == false) {
		log_error("Cannot register signal manager!");
		clean_workspace();
		return 1;
	}

	if (server_init() == false) {
		log_error("Server cannot be started.");
		clean_workspace();
		return 1;
	}

	if (server_start() == false) {
		log_fatal("Cannot start chatty server!");
		clean_workspace();
		return 1;
	}

	log_info("Welcome to chatty server!");
	log_debug("Server PID: %d", getpid());

	/*
	 * One time that server is started, while server is in
	 * running status, we can wait for incoming signals.
	 */
	while (server_status() == SERVER_STATUS_RUNNING) {
		log_trace("MAIN THREAD: waiting for server to finish");
		int action = signal_manager_wait();
		log_info("woke up from sleeping with signal %d", action);

		switch (action) {
		case SIGINT:
		case SIGQUIT:
			log_info("Received signal that stops server, quitting...");
			server_stop();
			break;
		case SIGUSR1:
			log_info("Printing statistics...");
			// print statistics
			break;
		default:
			// wait other signal
			break;

		}
	}

	log_debug("Waiting termination of server...");
	server_wait();
	log_debug("Cleaning up...");
	server_destroy();
	clean_workspace();
	log_info("Goodbye by chatty server!");

	return 0;
}

/**
 * This function checks if argument passed by system are valid or not.
 *
 * @param argc system argument count
 * @param argv system argument
 * @return true if arguments are valid, false otherwise
 */
bool check_arguments(int argc, char *argv[]) {
	if (argc < 3) {
		usage(argv[0]);
		return false;
	}

	if (strcmp(argv[1], "-f") != 0) {
		usage(argv[0]);
		return false;
	}

	log_debug("Arguments test passed");
	return true;
}

/**
 * Clean the workspace used by chatty
 */
void clean_workspace() {
	config_clean();
	log_destroy();
}
