/*******************************************************************************
 * SOL 2017/2018
 * Chatty
 * Federico Silvestri 559014
 * Si dichiara che il contenuto di questo file e' in ogni sua parte opera
 * originale dell'autore.
 *******************************************************************************/
/**
 * This file contains utility functions
 * to manage users and groups using SQLite3 Database.
 * Each function is thread-safe for definition.
 * @file userman.c
 */

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
 * This MACRO simplify code
 * during user list creation.
 * It calculates the size of user list.
 */
#define __USERLIST_SIZE(X) ((size_t)((MAX_NAME_LENGTH + 1) * ((int) sizeof(char)) * (X)))

static const char create_db_sql[] = "CREATE TABLE users("
		"nickname 			CHAR(%d) PRIMARY KEY     NOT NULL,"
		"last_login         DATETIME,"
		"online 			INTEGER );\n"
		"CREATE TABLE messages("
		"ID					INTEGER PRIMARY KEY AUTOINCREMENT,"
		"sender				CHAR(%d) NOT NULL,"
		"receiver			CHAR(%d) NOT NULL,"
		"timestamp			DATETIME,"
		"body				TEXT,"
		"is_file			INTEGER,"
		"read				INTEGER);";

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

static const char user_is_online_query[] = "SELECT EXISTS(SELECT 1 FROM users"
		" WHERE nickname = '%s' AND online='1')";

static const char message_add_query[] =
		"INSERT INTO messages(sender, receiver, timestamp, body, is_file, read) "
				"VALUES ('%s', '%s', time('now'), '%s', '%d', '%d') ";

static const char message_history_get[] = "SELECT body, is_file FROM messages "
		"WHERE receiver = '%s'";

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

/**
 * This function is a very simple sanitizer to avoid SQL Injection.
 * It replaces the escape chars, e.g. ' with '' or " with "".
 *
 * @param value to sanitize
 * @return number of occurences if success, -1 if error.
 */
static int simple_value_sanitizer(char **str) {
	if (*str == NULL) {
		return -1;
	}

	unsigned int rep = 0;
	for (unsigned int i = 0; i < strlen(*str) + 1; i++) {
		if (i == 0) {
			if ((*str)[i] == '\'') {
				rep += 1;
			}
		} else {
			if ((*str)[i - 1] != '\'' && (*str)[i] == '\'') {
				rep += 1;
			}
		}
	}

	// create another string with
	char *str_san = calloc(sizeof(char),
			sizeof(char) * (strlen(*str) + rep + 2));

	for (unsigned int i = 0, j = 0; i < strlen(*str); i++, j++) {
		if (i == 0) {
			if ((*str)[i] == '\'') {
				str_san[j] = '\'';
				str_san[j + 1] = (*str)[i];
				j += 1;
			} else {
				str_san[j] = (*str)[i];
			}
		} else {
			if ((*str)[i - 1] != '\'' && ((*str)[i] == '\'')) {
				str_san[j] = '\'';
				str_san[j + 1] = (*str)[i];
				j += 1;
			} else {
				str_san[j] = (*str)[i];
			}
		}
	}

	// free pointer
	free(*str);
	*str = str_san;
	return (int) rep;
}

static bool userman_create_db() {
	// creating table
	char *err_msg = NULL;

	char *sql = calloc(sizeof(char),
			sizeof(create_db_sql) + sizeof(char) * 9 * 3);
	sprintf(sql, create_db_sql, MAX_NAME_LENGTH, MAX_NAME_LENGTH,
	MAX_NAME_LENGTH);

	int rc = sqlite3_exec(db, sql, NULL, 0, &err_msg);

	log_info("Creating database from zero...");
	if (rc != SQLITE_OK) {
		log_fatal("Cannot create table on database: %s", err_msg);
		sqlite3_free(err_msg);
		free(sql);
		return false;
	}

	free(sql);
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

bool userman_user_is_online(char *nickname) {
	// query composition
	char *sql_query = calloc(sizeof(char),
			sizeof(user_is_online_query) + strlen(nickname) + 3);
	sprintf(sql_query, user_is_online_query, nickname);

	sqlite3_stmt *stmt = NULL;
	int rc = sqlite3_prepare_v2(db, sql_query, -1, &stmt, NULL);
	if (rc != SQLITE_OK) {
		log_fatal("Cannot prepare v2 statement!, %s", sqlite3_errmsg(db));
		return false; // return a sane value
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

bool userman_add_message(char *sender, char *receiver, bool read, char *body,
bool is_file) {
	// sanitize string values
	// copy body_c to similiar array to avoid memory inconvenient
	char *body_san = calloc(sizeof(char), strlen(body) + 1);
	strcpy(body_san, body);

	simple_value_sanitizer(&body_san);

	// query composition
	size_t query_size = sizeof(message_add_query); // standard query template
	query_size += sizeof(char) * (strlen(sender) + 1); // sender
	query_size += sizeof(char) * (strlen(receiver) + 1); // receiver
	query_size += sizeof(char) * 1; // bool value
	query_size += sizeof(char) * (strlen(body) + 1); // body size
	query_size += sizeof(char) * 1; // bool value

	// building query
	char *sql = calloc(sizeof(char), query_size);
	if (sql == NULL) {
		log_fatal("Out of memory during insert message query building...");
		return false;
	}
	sprintf(sql, message_add_query, sender, receiver, body_san, is_file, read);

	log_trace("Insert msg query: %s", sql);

	// query execution
	char *err_msg = NULL;
	int rc = sqlite3_exec(db, sql, NULL, 0, &err_msg);

	if (rc != SQLITE_OK) {
		log_fatal("SQL error: %s", err_msg);
		// cleanup
		sqlite3_free(err_msg);
		free(sql);
		free(body_san);
		return false;
	}

	// cleanup
	free(sql);
	free(body_san);
	return true;
}

int userman_get_prev_msgs(char *nickname, char ***list, bool **is_files) {
	// query building
	char *sql = NULL;
	size_t sql_size = sizeof(message_history_get);
	sql_size += sizeof(char) * (strlen(nickname) + 1);

	sql = calloc(sizeof(char), sql_size);

	if (sql == NULL) {
		log_fatal("Out of memory");
		return -1;
	}

	sprintf(sql, message_history_get, nickname);

	sqlite3_stmt *stmt = NULL;
	int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
	if (rc != SQLITE_OK) {
		log_fatal("Cannot prepare v2_statement %s", sqlite3_errmsg(db));
		return -1;
	}

	// return value
	int row_count = 0;
	rc = sqlite3_step(stmt);
	*list = calloc(sizeof(char**), sizeof(char**));
	*is_files = calloc(sizeof(bool), sizeof(bool));

	while (rc != SQLITE_DONE && rc != SQLITE_OK) {
		int colCount = sqlite3_column_count(stmt);

		for (int colIndex = 0; colIndex < colCount; colIndex++) {
			int type = sqlite3_column_type(stmt, colIndex);

			if (type == SQLITE_TEXT) {
				const char *body = (char*) sqlite3_column_text(stmt, colIndex);
				if (body == NULL) {
					log_fatal("Cannot retrieve field from SQLITE.");
					return -1;
				}

				(*list)[row_count] = malloc(sizeof(char) * (strlen(body) + 1));
				strcpy((*list)[row_count], body);
			} else if (type == SQLITE_INTEGER) {
				bool is_file = (bool) sqlite3_column_int(stmt, colIndex);
				(*is_files)[row_count] = is_file;
			} else {
				log_fatal(
						"Unexpected return value from sqlite during user selection");
				return -1;
			}
		}

		rc = sqlite3_step(stmt);
		row_count++;

		// check if need to realloc
		if (rc != SQLITE_DONE) {
			*list = realloc(*list, sizeof(char**) * (size_t) (row_count + 1));
			*is_files = realloc(*is_files,
					sizeof(bool*) * (size_t) (row_count + 1));
		}
	}

	// it frees the pointers
	rc = sqlite3_finalize(stmt);
	free(sql);

	return row_count;

}

void userman_destroy() {
	if (sqlite3_close(db) != SQLITE_OK) {
		log_warn("Cannot close database: %s", sqlite3_errmsg(db));
	}
}
