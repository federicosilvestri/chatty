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

#include "userman.h"

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <libconfig.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/signal.h>
#include <amqp.h>
#include <amqp_tcp_socket.h>
#include <sqlite3.h>

#include "log.h"
#include "message.h"
#include "config.h"
#include "amqp_utils.h"
#include "controller.h"
#include "worker.h"

/**
 * External configuration struct from config
 */
extern config_t server_conf;

/**
 * Database pathname
 */
static const char *db_pathname;

/**
 * Database handler
 */
static sqlite3 *db;

static bool userman_create_db() {
	// creating table
	const char sql[] = "CREATE TABLE USERS("
			"NICKNAME CHAR(120) PRIMARY KEY     NOT NULL,"
			"LAST_LOGIN         DATETIME );";
	char *err_msg = NULL;

	int rc = sqlite3_exec(db, sql, NULL, 0, &err_msg);

	if (rc != SQLITE_OK) {
		log_fatal("Cannot create table on database: %s", err_msg);
		sqlite3_free(err_msg);
		return false;
	}

	return true;
}

bool userman_init() {
	static bool init = false;

	if (init) {
		log_fatal("Userman initialized multiple times");
		return false;
	}

	// initialize parameter
	if (config_lookup_string(&server_conf, "DatabasePathname",
			&db_pathname) == CONFIG_FALSE) {
		log_fatal(
				"Programming error, cannot get default value of DatabasePathname");
		return false;
	}

	bool create_db = false;
	if (access(db_pathname, F_OK) == -1) {
		create_db = true;
	}

	// if exists open, if not exists create and open
	if (sqlite3_open(db_pathname, &db) != SQLITE_OK) {
		log_fatal("Cannot open database pathname: %s", sqlite3_errmsg(db));
		return false;
	}

	if (create_db) {
		if (userman_create_db() == false) {
			return false;
		}
	}

	return true;
}

void userman_destroy() {
	if (sqlite3_close(db) != SQLITE_OK) {
		log_warn("Cannot close database: %s", sqlite3_errmsg(db));
	}

}
