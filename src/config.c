/*
 * config.c
 *
 *  Created on: Aug 7, 2018
 *      Author: federicosilvestri
 */

/**
 * C POSIX source definition.
 */
#define _POSIX_C_SOURCE 200809L

#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <errno.h>
#include <libconfig.h>
#include <string.h>

#include "config.h"
#include "log.h"

/**
 * Configuration required parameters path
 */
static const char *config_req_params[] = { "UnixPath", "MaxConnections",
		"ThreadsInPool", "MaxMsgSize", "MaxFileSize", "MaxHistMsgs", "DirName",
		"StatFileName", "RabbitMQHostname", "RabbitMQPort" };

/**
 * Configuration required parameters type
 */
static const char config_req_params_type[] = { CONF_STRING_T, CONF_INT_T,
CONF_INT_T, CONF_INT_T, CONF_INT_T, CONF_INT_T, CONF_STRING_T,
CONF_STRING_T, CONF_STRING_T, CONF_INT_T};

/**
 * Size of array that contains required configuration
 * parameters
 */
#define CONFIG_REQUIRED_PARAMS_SIZE 10

/**
 * Main configuration container.
 */
config_t server_conf;


/**
 * @brief This function load into memory the file passed as configuration file.
 * If any problem occurs during loading it returns false.
 * @param conf_file_path string that represents the path of config file.
 *
 */
bool config_parse(char *conf_file_path) {
	// initialize configuration structure
	config_init(&server_conf);

	// try to load
	if (config_read_file(&server_conf, conf_file_path) != CONFIG_TRUE) {
		log_error("Configuration reading error on line %d: %s, %s",
				config_error_line(&server_conf),
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

/**
 * @brief clean workspace from any leaks.
 */
void config_clean() {
	// destroy config, only for now
	config_destroy(&server_conf);
}
