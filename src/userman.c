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

static const char user_insert_query[] =
		"INSERT INTO USERS (NICKNAME, LAST_LOGIN, ONLINE) "
				"VALUES ('%s', time('now'), '1')";

static const char user_exists_select_query[] =
		"SELECT EXISTS (SELECT 1 FROM USERS"
				" WHERE NICKNAME='%s' LIMIT 1)";

static const char user_get_query[] = "SELECT NICKNAME FROM USERS%s";

static const char user_get_query_online[] = " WHERE ONLINE = '1'";
static const char user_get_query_offline[] = " WHERE ONLINE = '0'";

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
			"LAST_LOGIN         DATETIME,"
			"ONLINE 			INTEGER );";
	char *err_msg = NULL;

	int rc = sqlite3_exec(db, sql, NULL, 0, &err_msg);

	log_info("Creating database from zero...");
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

	if (create_db == true) {
		if (userman_create_db() == false) {
			return false;
		}
	}

	return true;
}

//static int sqlite_exec_nc(const char *sql) {
//	sqlite3_stmt *stmt = NULL;
//	int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
//	if (rc != SQLITE_OK)
//		return rc;
//
//	int rowCount = 0;
//	rc = sqlite3_step(stmt);
//
//	while (rc != SQLITE_DONE && rc != SQLITE_OK) {
//		rowCount++;
//		int colCount = sqlite3_column_count(stmt);
//		for (int colIndex = 0; colIndex < colCount; colIndex++) {
//			int type = sqlite3_column_type(stmt, colIndex);
////			const char * columnName = sqlite3_column_name(stmt, colIndex);
//
//			if (type == SQLITE_INTEGER) {
////				int valInt = sqlite3_column_int(stmt, colIndex);
//
//			} else if (type == SQLITE_FLOAT) {
////				double valDouble = sqlite3_column_double(stmt, colIndex);
//
//			} else if (type == SQLITE_TEXT) {
////				const unsigned char *valChar = sqlite3_column_text(stmt,
////						colIndex);
////
//
//			} else if (type == SQLITE_BLOB) {
//
//			} else if (type == SQLITE_NULL) {
//
//			}
//		}
//
//		rc = sqlite3_step(stmt);
//	}
//
//	rc = sqlite3_finalize(stmt);
//	return rc;
//}

bool userman_user_exists(char *nickname) {
	// query composition
	char *sql_query = calloc(sizeof(char),
			sizeof(user_exists_select_query) + strlen(nickname) + 3);
	sprintf(sql_query, user_exists_select_query, nickname);

	sqlite3_stmt *stmt = NULL;
	int rc = sqlite3_prepare_v2(db, sql_query, -1, &stmt, NULL);
	if (rc != SQLITE_OK) {
		log_fatal("Cannot prepare v2 statement!, %s", sqlite3_errmsg(db));
		return true; // return a sane value
	}

	rc = sqlite3_step(stmt);
	int ret_value = 1;

	while (rc != SQLITE_DONE && rc != SQLITE_OK) {
		int colCount = sqlite3_column_count(stmt);
		for (int colIndex = 0; colIndex < colCount; colIndex++) {
			int type = sqlite3_column_type(stmt, colIndex);

			if (type == SQLITE_INTEGER) {
				ret_value = sqlite3_column_int(stmt, colIndex);
			} else {
				log_fatal("Unexpected response from query!");
				exit(1);
			}
		}

		rc = sqlite3_step(stmt);
	}
	rc = sqlite3_finalize(stmt);

	// cleanup
	free(sql_query);

	return (ret_value == 1);
}

int userman_add_user(char *nickname) {
	// check first if exists
	if (userman_user_exists(nickname) == true) {
		return 1;
	}

	if (nickname == NULL) {
		return 2;
	}

	// prepare query
	char *sql_query = calloc(sizeof(char),
			sizeof(user_insert_query) + strlen(nickname) + 3);

	sprintf(sql_query, user_insert_query, nickname);

	char *err_msg = NULL;
	int rc = sqlite3_exec(db, sql_query, NULL, 0, &err_msg);

	if (rc != SQLITE_OK) {
		log_fatal("SQL error: %s", err_msg);
		// cleanup
		sqlite3_free(err_msg);
		free(sql_query);
		return 3;
	}

	// cleanup
	free(sql_query);

	return 0;
}

int userman_get_users(char option, char **list, int *ss) {
	// composing query

	char *sql_ext = NULL;

	switch (option) {
	case USERMAN_GET_ALL:
		sql_ext = calloc(sizeof(char), sizeof(user_get_query) + 1);
		sprintf(sql_ext, user_get_query, "");
		break;
	case USERMAN_GET_ONL:
		sql_ext = calloc(sizeof(char),
				sizeof(user_get_query) + sizeof(user_get_query_online));
		sprintf(sql_ext, user_get_query, user_get_query_online);
		break;
	case USERMAN_GET_OFFL:
		sql_ext = calloc(sizeof(char),
				sizeof(user_get_query) + sizeof(user_get_query_offline));
		sprintf(sql_ext, user_get_query, user_get_query_offline);
		break;
	default:
		log_fatal(
				"Programming error, passed a bad option to userman_get_users");
		return -1;
	}

	sqlite3_stmt *stmt = NULL;
	int rc = sqlite3_prepare_v2(db, sql_ext, -1, &stmt, NULL);
	if (rc != SQLITE_OK) {
		log_fatal("Cannot prepare v2_statement %s", sqlite3_errmsg(db));
		return -1;
	}

	// return value
	int row_count = 0;
	rc = sqlite3_step(stmt);
	*list = calloc(sizeof(char), sizeof(char) * (MAX_NAME_LENGTH + 1));
	size_t list_size = sizeof(char) * (MAX_NAME_LENGTH + 1);

	while (rc != SQLITE_DONE && rc != SQLITE_OK) {
		row_count++;
		int colCount = sqlite3_column_count(stmt);

		for (int colIndex = 0; colIndex < colCount; colIndex++) {
			int type = sqlite3_column_type(stmt, colIndex);

			if (type == SQLITE_TEXT) {
				const unsigned char *nickname = sqlite3_column_text(stmt,
						colIndex);

				// check if memory is needed
				if (list_size
						< (sizeof(char) * row_count * (MAX_NAME_LENGTH + 1))) {
					// executing realloc
					*list = realloc(*list,
							(sizeof(char) * row_count * (MAX_NAME_LENGTH + 1)));
					list_size += (sizeof(char) * (MAX_NAME_LENGTH + 1));

					// initialize memory
//					memset(*list,
//							sizeof(char) * (MAX_NAME_LENGTH + 1)
//									* (row_count - 1), list_size);

					if (*list == NULL) {
						log_fatal("OUT OF MEMORY");
						exit(1);
					}
				}

				// build the list by position
				const int s_len = sizeof(char) * (MAX_NAME_LENGTH + 1);

				for (int i = s_len * (row_count - 1), j = 0;
						j <= strlen((char *) nickname); i++, j++) {
					if (j == strlen((char *) nickname)) {
						(*list)[i] = '\0';
					} else {
						(*list)[i] = nickname[j];
					}

					log_trace("[] -> %c", (*list)[i]);

				}

			} else {
				log_fatal(
						"Unexpected return value from sqlite during user selection");
				return -1;
			}
		}

		rc = sqlite3_step(stmt);
	}

	// it frees the pointers
	rc = sqlite3_finalize(stmt);
	*ss = list_size;

	// cleanup
	free(sql_ext);

	log_trace("Size of list %d", list_size);

	return row_count;
}

void userman_destroy() {
	if (sqlite3_close(db) != SQLITE_OK) {
		log_warn("Cannot close database: %s", sqlite3_errmsg(db));
	}

}
