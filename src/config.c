/*******************************************************************************
 * SOL 2017/2018
 * Chatty
 * Federico Silvestri 559014
 * Si dichiara che il contenuto di questo file e' in ogni sua parte opera
 * originale dell'autore.
 *******************************************************************************/

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
static const char config_req_params_type[] = { CONFIG_TYPE_STRING, CONFIG_TYPE_INT,
CONFIG_TYPE_INT, CONFIG_TYPE_INT, CONFIG_TYPE_INT, CONFIG_TYPE_INT, CONFIG_TYPE_STRING,
CONFIG_TYPE_STRING, CONFIG_TYPE_STRING, CONFIG_TYPE_INT };

/**
 * Configuration optional parameters path
 */
static const char *config_opt_params[] =
		{ "RabbitMQExchange", "RabbitMQBindKey" };

/**
 * Configuration optional parameters type
 */
static const char config_opt_params_type[] = { CONFIG_TYPE_STRING, CONFIG_TYPE_STRING };

/**
 * Configuration optional parameters default value
 */
static const void *config_opt_params_default_value[] = { "EX", "E32838dck" };

/**
 * Size of array that contains required configuration
 * parameters
 */
#define CONFIG_REQUIRED_PARAMS_SIZE 10

/**
 * Size of optional parameters.
 */
#define CONFIG_OPTIONAL_PARAMS_SIZE 2

/**
 * Main configuration container.
 */
config_t server_conf;

/**
 * Load default configuration for optional parameter
 * @param index of parameter
 * @param type of parameter
 */

static void config_load_default(int i, char type) {
	struct config_setting_t *root, *added;

	root = config_root_setting(&server_conf);
	added = config_setting_add(root, config_opt_params[i], type);

	switch(type) {
	case CONFIG_TYPE_STRING:
		config_setting_set_string(added, config_opt_params_default_value[i]);
		break;
	case CONFIG_TYPE_INT:
		config_setting_set_string(added, config_opt_params_default_value[i]);
		break;
	}
}

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
		case CONFIG_TYPE_STRING:
			if (config_lookup_string(&server_conf, config_req_params[i],
					&str_value) == CONFIG_FALSE) {
				miss_param = i;
			}
			break;
		case CONFIG_TYPE_INT:
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

	// checking optional parameters and load default if missing
	for (int i = 0; i < CONFIG_OPTIONAL_PARAMS_SIZE; i++) {
		const char *str_value;
		int int_value = 0;

		switch (config_opt_params_type[i]) {
		case CONFIG_TYPE_STRING:
			if (config_lookup_string(&server_conf, config_opt_params[i],
					&str_value) == CONFIG_FALSE) {
				config_load_default(i, CONFIG_TYPE_STRING);
			}
			break;
		case CONFIG_TYPE_INT:
			if (config_lookup_int(&server_conf, config_opt_params[i],
					&int_value) == CONFIG_FALSE) {
				config_load_default(i, CONFIG_TYPE_INT);
			}
			break;
		}
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
