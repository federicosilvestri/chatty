/*******************************************************************************
 * SOL 2017/2018
 * Chatty
 * Federico Silvestri 559014
 * Si dichiara che il contenuto di questo file e' in ogni sua parte opera
 * originale dell'autore.
 *******************************************************************************/

/**
 * This file manages the runtime statistics of
 * server.
 * @file stats.c
 */

/**
 * C POSIX source definition.
 */
#define _POSIX_C_SOURCE 200809L

#include "stats.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <libconfig.h>
#include <libgen.h>
#include <pthread.h>

#include "log.h"
#include "config.h"

/**
 * External configuration by libconfig
 */
extern config_t server_conf;

/**
 * Filename of statistics file.
 */
static const char *stat_filename;

/**
 * Mutex for statics write
 */
static pthread_mutex_t stat_mutex;

/**
 * Variable to register statistics.
 */
struct statistics chattyStats = { 0, 0, 0, 0, 0, 0, 0 };

bool stats_init() {
	bool initialized = false;
	if (initialized) {
		return false;
	}
	initialized = true;

	// get statistics filename
	if (config_lookup_string(&server_conf, "StatFileName",
			&stat_filename) == CONFIG_FALSE) {
		log_fatal("Cannot get value for StatFileName!");
		return false;
	}

	// check the name
	if (strlen(stat_filename) <= 0) {
		log_fatal("Bad statistics file name.");
		return false;
	}

	// check if file dir is writable
	char *dir = strdup(stat_filename);
	dirname(dir);

	if (access(dir, W_OK) != 0) {
		log_fatal(
				"Stat filename is not accessible with write privileges! file=%s",
				stat_filename);
		return false;
	}

	free(dir);

	// initialize mutex
	if (pthread_mutex_init(&stat_mutex, NULL) != 0) {
		log_fatal("Cannot initialize mutex! System fail.");
		return false;
	}

	return true;
}

void stats_trigger() {
	pthread_mutex_lock(&stat_mutex);
	// open stat file
	FILE *file_handle = fopen(stat_filename, "w");

	if (file_handle == NULL) {
		log_error("Cannot open file! error=%s", strerror(errno));
		return;
	}

	// print statistics
	if (printStats(file_handle) == -1) {
		log_error("Cannot write to statistics file, error=%s", strerror(errno));
	}

	pthread_mutex_unlock(&stat_mutex);
	fclose(file_handle);

	log_info("Statistics file=%s GENERATED", stat_filename);
}


void stats_update_value(unsigned long add, unsigned long remove, unsigned long *dest) {
	pthread_mutex_lock(&stat_mutex);

	*dest += add;

	if (*dest < remove) {
		*dest = 0;
	} else {
		*dest -= remove;
	}

	pthread_mutex_unlock(&stat_mutex);
}


void stats_destroy() {
	// destroy the used mutex
	pthread_mutex_destroy(&stat_mutex);
}
