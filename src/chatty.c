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
#include "signal_handler.h"

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
	if (check_arguments(argc, argv) == false) {
		return 1;
	}

	if (config_parse(argv[2]) == false) {
		return 1;
	}

	// send initialization to signal handler
	signal_handler_init();

	if (signal_handler_register() == false) {
		clean_workspace();
		return 1;
	}

	if (server_start() == false) {
		log_error("Cannot start chatty server!");
	} else {
		log_info("Welcome to chatty server!");
	}

	// while server is in running status
	while (server_status() == SERVER_STATUS_RUNNING) {
		// put server controller thread in wait queue
		server_join();
	}

	log_debug("Cleaning up...");
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

void clean_workspace() {
	config_clean();
}
