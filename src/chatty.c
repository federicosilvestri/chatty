/*
 * membox Progetto del corso di LSO 2017/2018
 *
 * Dipartimento di Informatica Università di Pisa
 * Docenti: Prencipe, Torquati
 * 
 */
/**
 * @file chatty.c
 * @brief File principale del server chatterbox
 */
#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <signal.h>
#include <pthread.h>

/* inserire gli altri include che servono */
#include "chatty.h"

struct statistics chattyStats = { 0, 0, 0, 0, 0, 0, 0 };

static void usage(const char *progname) {
	fprintf(stderr, "Il server va lanciato con il seguente comando:\n");
	fprintf(stderr, "  %s -f <configuration-file>\n", progname);
}

int main(int argc, char *argv[]) {
	if (check_arguments(argc, argv) == false) {
		return 1;
	}

	if (parse_config(argv[2]) == false) {
		return 1;
	}

	return 0;
}

/**
 * This function checks if argument passed by system are valid or not.
 *
 * @param argc system argument count
 * @param argv system argument
 * @return true if arguments are valid, false otherwise
 */
static inline bool check_arguments(int argc, char *argv[]) {
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
 * @brief This function load into memory the file passed as configuration file.
 * If any problem occurs during loading it returns false.
 *
 *
 */
bool parse_config(char conf_file_path[]) {
	// initialize configuration structure
	config_init(&server_conf);

	// try to load
	if (config_read_file(&server_conf, conf_file_path) != CONFIG_TRUE) {
		log_error("Configuration reading error: %s, %s",
				config_error_text(&server_conf), strerror(errno));

		config_destroy(&server_conf);

		return false;
	}

	log_debug("Configuration file loaded");

	// checking if all mandatory parameters are specified
	int miss_param = -1;

	for (int i = 0; i < CONFIG_REQUIRED_PARAMS_SIZE && miss_param == -1; i++) {
		const char *str_value;
		int int_value = 0;

		log_debug("Checking %s on file", config_req_params[i]);

		switch (config_req_params_type[i]) {
		case CONF_STRING_T:
			if (config_lookup_string(&server_conf, config_req_params[i],
					&str_value) == CONFIG_FALSE) {
				miss_param = i;
			}
			break;
		case CONF_INT_T:
			if (config_lookup_int(&server_conf, config_req_params[i],
					&int_value) == CONFIG_FALSE) {
				miss_param = i;
			}
			break;
		}
	}

	if (miss_param != -1) {
		log_error(
				"Configuration test failed: missing mandatory configuration parameter \"%s\"",
				config_req_params[miss_param]);
		config_destroy(&server_conf);

		return false;
	}

	log_debug("Configuration file test passed");
	return true;
}
