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

#define __USERLIST_SIZE(X) ((size_t)((MAX_NAME_LENGTH + 1) * ((int) sizeof(char)) * (X)))

static const char create_db_sql[] = "CREATE TABLE users("
		"nickname CHAR(120) PRIMARY KEY     NOT NULL,"
		"last_login         DATETIME,"
		"online 			INTEGER );";

static const char user_insert_query[] =
		"INSERT INTO users (nickname, last_login, online) "
				"VALUES ('%s', time('now'), '0')";

static const char user_delete_query[] =
		"DELETE FROM users WHERE nickname = '%s' LIMIT 1";

static const char user_exists_select_query[] =
		"SELECT EXISTS (SELECT 1 FROM users"
				" WHERE nickname='%s' LIMIT 1)";

static const char user_get_query[] = "SELECT nickname FROM users%s";

static const char user_get_online_query[] = " WHERE online = '1'";

static const char user_get_offline_query[] = " WHERE online = '0'";

static const char user_set_status_query[] =
		"UPDATE users SET online = '%d' WHERE nickname = '%s'";

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
	char *err_msg = NULL;

	int rc = sqlite3_exec(db, create_db_sql, NULL, 0, &err_msg);

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
	if (nickname == NULL) {
			return 2;
		}

	if (userman_user_exists(nickname) == true) {
		return 1;
	}

	if (strlen(nickname) > MAX_NAME_LENGTH) {
		return 3;
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
		return 4;
	}

	// cleanup
	free(sql_query);

	return 0;
}

bool userman_delete_user(char *nickname) {
	// check first if exists
	if (nickname == NULL) {
		return false;
	}

	if (userman_user_exists(nickname) == false) {
		return false;
	}

	// prepare query
	char *sql_query = calloc(sizeof(char),
			sizeof(user_delete_query) + strlen(nickname) + 3);

	sprintf(sql_query, user_delete_query, nickname);

	char *err_msg = NULL;
	int rc = sqlite3_exec(db, sql_query, NULL, 0, &err_msg);

	if (rc != SQLITE_OK) {
		log_fatal("SQL error: %s", err_msg);
		// cleanup
		sqlite3_free(err_msg);
		free(sql_query);
		return false;
	}

	// cleanup
	free(sql_query);

	return true;
}

size_t userman_get_users(char option, char **list) {
	// query composition (binding)
	char *sql_ext = NULL;

	switch (option) {
	case USERMAN_GET_ALL:
		sql_ext = calloc(sizeof(char), sizeof(user_get_query) + 1);
		sprintf(sql_ext, user_get_query, "");
		break;
	case USERMAN_GET_ONL:
		sql_ext = calloc(sizeof(char),
				sizeof(user_get_query) + sizeof(user_get_online_query));
		sprintf(sql_ext, user_get_query, user_get_online_query);
		break;
	case USERMAN_GET_OFFL:
		sql_ext = calloc(sizeof(char),
				sizeof(user_get_query) + sizeof(user_get_offline_query));
		sprintf(sql_ext, user_get_query, user_get_offline_query);
		break;
	default:
		log_fatal(
				"Programming error, passed a bad option to userman_get_users");
		return 0;
	}

	sqlite3_stmt *stmt = NULL;
	int rc = sqlite3_prepare_v2(db, sql_ext, -1, &stmt, NULL);
	if (rc != SQLITE_OK) {
		log_fatal("Cannot prepare v2_statement %s", sqlite3_errmsg(db));
		return 0;
	}

	// return value
	int row_count = 0;
	rc = sqlite3_step(stmt);
	*list = calloc(sizeof(char), __USERLIST_SIZE(1));
	size_t list_size = __USERLIST_SIZE(1);

	while (rc != SQLITE_DONE && rc != SQLITE_OK) {
		row_count++;
		int colCount = sqlite3_column_count(stmt);

		for (int colIndex = 0; colIndex < colCount; colIndex++) {
			int type = sqlite3_column_type(stmt, colIndex);

			if (type == SQLITE_TEXT) {
				const unsigned char *nickname = sqlite3_column_text(stmt,
						colIndex);

				// check if memory is needed
				if (list_size < __USERLIST_SIZE(row_count)) {
					// executing realloc
					*list = realloc(*list, __USERLIST_SIZE(row_count));

					list_size += __USERLIST_SIZE(1);

					if (*list == NULL) {
						log_fatal("OUT OF MEMORY");
						exit(1);
					}

				}

				// build the list by position
				const int s_len = __USERLIST_SIZE(row_count - 1);

				char *p = &((*list)[s_len]);
				strncpy(p, (char *) nickname, MAX_NAME_LENGTH + 1);

			} else {
				log_fatal(
						"Unexpected return value from sqlite during user selection");
				return 0;
			}
		}

		rc = sqlite3_step(stmt);
	}

	// it frees the pointers
	rc = sqlite3_finalize(stmt);
	free(sql_ext);

	return list_size;
}

bool userman_set_user_status(char *nickname, bool status) {
	if (nickname == NULL) {
		return false;
	}

	bool ret = true;
	char *sql = calloc(sizeof(char),
			sizeof(user_set_status_query) + 1 + strlen(nickname) + 1);
	sprintf(sql, user_set_status_query, status, nickname);

	char *sql_errmsg;
	int rc = sqlite3_exec(db, sql, NULL, 0, &sql_errmsg);

	if (rc != SQLITE_OK) {
		log_fatal("Programming error, query execution failed: %s, %s",
				sql_errmsg, sqlite3_errmsg(db));

		// cleanup error
		sqlite3_free(sql_errmsg);
		ret = false;
	}

	// cleanup query
	free(sql);

	return ret;
}

void userman_destroy() {
	if (sqlite3_close(db) != SQLITE_OK) {
		log_warn("Cannot close database: %s", sqlite3_errmsg(db));
	}

}
