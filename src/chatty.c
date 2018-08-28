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
	fprintf(stderr, "  %s -f <configuration-file> [ -v 1|2|3|4|5|6|7 ]\n",
			progname);
	fprintf(stderr,
			"  -v means verbose, 1 means TRACE, 7 means QUITE(suppress all messages)\n");
}

/**
 * Main function that is called from system.
 * @param argc argument count
 * @param argv array of string that contains arguments
 * @return program exit code.
 */
int main(int argc, char *argv[]) {
	int log_level = STD_LOG_LEVEL;

	if (checkandget_arguments(argc, argv, &log_level) == false) {
		return 1;
	}

	// set the log level
	log_init(log_level - 1);

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
	bool brutal_kill = false;
	int stop_send = 0;
	while (server_status() == SERVER_STATUS_RUNNING && !brutal_kill) {
		log_trace("MAIN THREAD: waiting for server to finish");
		int action = signal_manager_wait();
		log_info("woke up from sleeping with signal %d", action);

		switch (action) {
		case SIGINT:
		case SIGTERM:
		case SIGQUIT: {
			if (server_status() != SERVER_STATUS_STOPPED) {
				if (stop_send == 0) {
					log_info("Received signal that stops server, quitting...");
					server_stop();
					stop_send += 1;
				} else if (stop_send < 2){
					log_warn("Keep calm! Server is stopping... If you want to kill now press again");
					stop_send += 1;
				} else {
					// brutally kill
					brutal_kill = true;
				}
			}
			break;
		}
		case SIGUSR1:
			log_info("Printing statistics...");
			// print statistics
			break;
		default:
			// wait other signal
			break;

		}
	}

	if (brutal_kill) {
		log_error("Server is killed. Possible memory leaks.");
		return 1;
	}

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
static bool checkandget_arguments(int argc, char *argv[], int* log_level) {
	if (argc < 3 || argc == 4) {
		usage(argv[0]);
		return false;
	}

	if (strcmp(argv[1], "-f") != 0) {
		usage(argv[0]);
		return false;
	}

	if (argc == 5) {
		// log configuration detected
		if (strcmp(argv[3], "-v") != 0) {
			usage(argv[0]);
			return false;
		}

		// get the log level
		*log_level = atoi(argv[4]);
		if (*log_level < 1 || *log_level > 7) {
			usage(argv[0]);
			return false;
		}
	}

	return true;
}

/**
 * Clean the workspace used by chatty
 */
static void clean_workspace() {
	config_clean();
	log_destroy();
}
