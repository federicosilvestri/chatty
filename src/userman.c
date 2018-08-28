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
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
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

static const char message_get_query[] =
		"SELECT id, is_file, body, sender FROM messages "
				"WHERE receiver = '%s' AND read LIKE '%c' ORDER BY ID DESC LIMIT %d";

static const char message_set_status_query[] = "UPDATE messages SET read = '%d'"
		" WHERE id = '%d'";

static const char file_exists_query[] = "SELECT EXISTS(SELECT 1 FROM messages "
		"WHERE receiver='%s' AND body = '%s' AND is_file = '1')";

/**
 * External configuration struct from config
 */
extern config_t server_conf;

/**
 * Database pathname
 */
static const char *db_pathname;

/**
 * File directory path.
 */
static const char *filedir_path;

/**
 * Mutex used in some cases
 * during writing files on datastore.
 */
static pthread_mutex_t filedir_mutex;

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

/**
 * This function isolates the file name from path.
 * For example /etc/cassandra/conf.yaml returns conf.yaml
 * @param str the string to isolate
 */
static char* isolate_filename(char *str) {
	if (str == NULL || strlen(str) <= 0) {
		return NULL;
	}

	char *p = NULL;
	int i;
	for (i = (int) strlen(str) - 1; i > 0; i--) {
		if (str[i] == '/') {
			p = &str[i + 1];
			break;
		}
	}

	if (p == NULL) {
		return strdup(str);
	}

	size_t adj_size = sizeof(char)
			* (((size_t) strlen(str)) - (size_t) (i) + 2);
	char *adj = calloc(sizeof(char), adj_size);

	strcpy(adj, p);
	return adj;
}

/**
 * Returns the file path for a file.
 * @param str the name of the file
 * @return the pointer (to be freed) of string
 */
static char *get_filepath(char *filename) {
	size_t string_size = sizeof(char)
			* (strlen(filedir_path) + strlen(filename) + 3);
	char *s_file_pathname = calloc(sizeof(char), string_size);
	strcpy(s_file_pathname, filedir_path);

	// concat string filename
	if (filedir_path[strlen(filedir_path)] != '/') {
		strcat(s_file_pathname, "/");
	}
	strcat(s_file_pathname, filename);

	return s_file_pathname;
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

	// Get file directory
	if (config_lookup_string(&server_conf, "DirName",
			&filedir_path) == CONFIG_FALSE) {
		log_fatal("Cannot retrieve DirName configuration param!");
		return false;
	}

	if (strlen(filedir_path) <= 0) {
		log_fatal("DirName parameter is not valid!");
		return false;
	}

	// temporary struct for directory checking
	struct stat stat_buffer;
	if (stat(filedir_path, &stat_buffer) != 0) {
		// cannot retrieve information
		log_fatal("Cannot retrieve information about DirName=%s directory!",
				filedir_path);
		return false;
	}

	if (S_ISDIR(stat_buffer.st_mode) == 0) {
		log_fatal("DirName=%s is not a directory!");
		return false;
	}

	if (access(filedir_path, W_OK) != 0) {
		log_fatal("DirName=%s is not writable!");
		return false;
	}

	// init mutex for datastore
	check_mutex_lu_call(pthread_mutex_init(&filedir_mutex, NULL));

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
				free(sql_query);
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

	if (strlen(nickname) <= 0) {
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

int userman_get_users(char option, char **list) {
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
	bool stop_err = false;

	while (rc != SQLITE_DONE && rc != SQLITE_OK && !stop_err) {
		row_count++;
		int colCount = sqlite3_column_count(stmt);

		for (int colIndex = 0; colIndex < colCount && !stop_err; colIndex++) {
			int type = sqlite3_column_type(stmt, colIndex);

			if (type == SQLITE_TEXT) {
				const char *nickname = (char*) sqlite3_column_text(stmt,
						colIndex);

				if (strlen(nickname) <= 0) {
					log_error(
							"Found null nickname in the database. Is database corrupted?");
					stop_err = true;
				}

				// check if memory is needed
				if (list_size < __USERLIST_SIZE(row_count)) {
					// executing realloc
					*list = realloc(*list, __USERLIST_SIZE(row_count));

					list_size += __USERLIST_SIZE(1);

					if (*list == NULL) {
						log_fatal("OUT OF MEMORY");
						stop_err = true;
					}

				}

				// build the list by position
				const int s_len = __USERLIST_SIZE(row_count - 1);

				char *p = &((*list)[s_len]);
				strncpy(p, (char *) nickname, MAX_NAME_LENGTH + 1);

			} else {
				log_fatal(
						"Unexpected return value from sqlite during user selection");
				stop_err = true;
			}
		}

		rc = sqlite3_step(stmt);
	}

	// it frees the pointers
	rc = sqlite3_finalize(stmt);
	free(sql_ext);

	return (stop_err == true) ? 0 : (int) list_size;
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

	if (is_file == true) {
		// extract file name
		char *isolated = isolate_filename(body_san);

		if (strcmp(isolated, body_san) == 0) {
			// no problem
			free(isolated);
		} else {
			char *t = body_san;
			body_san = isolated;
			free(t);
		}
	}

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

bool userman_add_broadcast_msg(char *nickname, bool is_file, char *body) {
	if (nickname == NULL || body == NULL) {
		return false;
	}

	/*
	 * Simple algorithm, add a message for all users.
	 */

	// first retrieve user list
	char *nicknames = NULL;
	int users_n = (int) userman_get_users(USERMAN_GET_ALL, &nicknames);

	if (users_n < 0) {
		free(nicknames);
		log_fatal("Cannot continue due to previous error.");
		return false;
	}

	// for each user, send message
	bool stop_error = false;
	users_n /= MAX_NAME_LENGTH + 1;
	for (int i = 0, p = 0; i < users_n && !stop_error;
			i++, p += MAX_NAME_LENGTH + 1) {
		char *user_nick = &nicknames[p];

		log_trace("nickname=%s", user_nick);
		if (strcmp(user_nick, nickname) == 0) {
			// do not send message to itself
			continue;
		}

		// add message to user
		bool add_ok = userman_add_message(nickname, user_nick, false, body,
				is_file);

		if (!add_ok) {
			log_fatal("Cannot add message to user!");
			stop_error = true;
		}
	}

	// free memory
	free(nicknames);

	return !stop_error;
}

int userman_get_msgs(char *nickname, char ***list, char ***senders, int **ids,
bool **is_files, unsigned short int read_m, int limit) {
	// query building
	char *sql = NULL;
	size_t sql_size = sizeof(message_get_query);
	sql_size += sizeof(char) * (strlen(nickname) + 1);
	sql_size += sizeof(char) * 1; // read or unread value
	sql_size += sizeof(char) * 8; // max size of limit number

	sql = calloc(sizeof(char), sql_size);

	if (sql == NULL) {
		log_fatal("Out of memory");
		return -1;
	}

	char read_v = '0';
	switch (read_m) {
	case USERMAN_GET_MSGS_ALL:
		read_v = '%';
		break;
	case USERMAN_GET_MSGS_READ:
		read_v = '1';
		break;
	case USERMAN_GET_MSGS_UNREAD:
		read_v = '0';
		break;
	default:
		log_fatal("Bad read parameters passed to function!");
		free(sql);
		break;
	}

	sprintf(sql, message_get_query, nickname, read_v, limit);

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

	if (senders != NULL) {
		*senders = calloc(sizeof(char**), sizeof(char**));
	}

	if (ids != NULL) {
		*ids = calloc(sizeof(int), sizeof(int));
	}

	while (rc != SQLITE_DONE && rc != SQLITE_OK) {
		int colCount = sqlite3_column_count(stmt);

		for (int colIndex = 0; colIndex < colCount; colIndex++) {
			int type = sqlite3_column_type(stmt, colIndex);

			if (type == SQLITE_TEXT) {
				const char *str_v = (char*) sqlite3_column_text(stmt, colIndex);

				if (str_v == NULL) {
					log_fatal("Cannot retrieve field from SQLITE.");
					return -1;
				}

				if (colIndex == 2) {
					(*list)[row_count] = malloc(
							sizeof(char) * (strlen(str_v) + 1));
					strcpy((*list)[row_count], str_v);
				} else {
					if (senders != NULL) {
						(*senders)[row_count] = malloc(
								sizeof(char) * (strlen(str_v) + 1));
						strcpy((*senders)[row_count], str_v);
					}
				}
			} else if (type == SQLITE_INTEGER) {
				int i_value = sqlite3_column_int(stmt, colIndex);

				// check the index of query (very simple but rude)
				if (colIndex == 0) {
					// id field
					if (ids != NULL) {
						(*ids)[row_count] = i_value;
					}
				} else {
					// read field
					(*is_files)[row_count] = (bool) i_value;
				}
			} else {
				log_fatal(
						"Unexpected return value from sqlite during user selection");
				userman_free_msgs(list, senders, row_count, ids, is_files);

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
			if (ids != NULL) {
				*ids = realloc(*ids, sizeof(int*) * (size_t) (row_count + 1));
			}

			if (senders != NULL) {
				*senders = realloc(*senders,
						sizeof(char **) * (size_t) (row_count + 1));
			}
		}
	}

	// it frees the pointers
	rc = sqlite3_finalize(stmt);
	free(sql);

	// check if no columns are found, free pointers
	if (row_count == 0) {
		free(*is_files);
		free(*list);

		if (ids != NULL) {
			free(*ids);
		}

		if (senders != NULL) {
			free(*senders);
		}
	}

	return row_count;

}

void userman_free_msgs(char ***list, char ***senders, int list_size, int **ids,
bool **is_files) {
	if (list_size <= 0) {
		// all memory was freed by userman_get_msgs function
		return;
	}

	if (list != NULL) {
		for (int i = 0; i < list_size; i++) {
			free(*list[i]);
		}

		free(*list);

	}

	if (senders != NULL) {
		for (int i = 0; i < list_size; i++) {
			free(*senders[i]);
		}

		free(*list);
	}

	if (ids != NULL) {
		free(*ids);
	}

	if (is_files != NULL) {
		free(is_files);
	}
}

bool userman_set_msg_status(int msgid, bool read) {
	if (msgid < 0) {
		return false;
	}

	bool ret = true;
	size_t sql_size = sizeof(message_set_status_query);
	sql_size += sizeof(char) * 10 * 2; // integer numbers.

	char *sql = calloc(sizeof(char), sql_size);
	sprintf(sql, message_set_status_query, read, msgid);

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

bool userman_store_file(char *ufilename, char *buffer, unsigned int bufflen) {
	if (ufilename == NULL || buffer == NULL || bufflen == 0) {
		return false;
	}

	if (strlen(ufilename) <= 0) {
		return false;
	}

	char *filename = isolate_filename(ufilename);
	char *s_file_pathname = get_filepath(filename);
	free(filename);

	log_debug("Writing filepath=%s", s_file_pathname);

	// do we need to touch file?

	// check if file already exists
	if (access(s_file_pathname, F_OK) == 0) {
		log_warn("File already exists! Overwriting...");
	}

	/*
	 * We need to open a file for writing
	 * and write the file.
	 */

	// LOCK the write to this thread
	check_mutex_lu_call(pthread_mutex_lock(&filedir_mutex));

	bool write_error = false;
	FILE *s_file_handle = NULL;
	s_file_handle = fopen(s_file_pathname, "wb");

	if (s_file_handle == NULL) {
		log_fatal("Cannot open file for writing... error=%s", strerror(errno));
		write_error = true;
	} else {
		size_t write_size = fwrite(buffer, sizeof(char), bufflen,
				s_file_handle);

		if (write_size != bufflen) {
			log_error("Cannot write the received file, error=%s",
					strerror(errno));
			write_error = true;
		}

		fclose(s_file_handle);
	}

	// UNLOCK the write
	check_mutex_lu_call(pthread_mutex_unlock(&filedir_mutex));

	// free memory
	free(s_file_pathname);

	return !write_error;
}

bool userman_file_exists(char *filename, char *nickname) {
	if (filename == NULL || strlen(filename) <= 0) {
		return false;
	}

	if (nickname == NULL || strlen(nickname) <= 0) {
		return false;
	}

	// compose query
	size_t sql_size = sizeof(file_exists_query);
	sql_size += sizeof(char) * ((size_t) (strlen(nickname)) + 1);
	sql_size += sizeof(char) * ((size_t) (strlen(filename)) + 1);

	char *sql_query = calloc(sizeof(char), sql_size);
	sprintf(sql_query, file_exists_query, nickname, filename);

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
				free(sql_query);
				exit(1);
			}
		}

		rc = sqlite3_step(stmt);
	}

	rc = sqlite3_finalize(stmt);
	free(sql_query);

	return (ret_value == 1);
}

bool userman_search_file(char *filename) {
	if (filename == NULL || strlen(filename) <= 0) {
		return false;
	}

	bool ret = false;
	char *adj = isolate_filename(filename);
	char *filepath = get_filepath(adj);
	free(adj);

	if (access(filepath, F_OK) == 0) {
		ret = true;
	}

	free(filepath);

	return ret;
}

size_t userman_get_file(char *filename, char **buffer) {
	if (filename == NULL) {
		return (size_t) -1;
	}

	// preparing the real path
	char *adj = isolate_filename(filename);
	char *real_path = get_filepath(adj);
	free(adj);

	struct stat st;
	if (stat(real_path, &st) == -1) {
		log_fatal("Cannot execute stat to file! error=%s", strerror(errno));
		free(real_path);
		return (size_t) -1;
	}

	int file_fd = open(real_path, O_RDONLY);

	if (file_fd < 0) {
		log_fatal("Cannot open file! error=%s", strerror(errno));
		free(real_path);
		return (size_t) -1;

	}

	*buffer = mmap(NULL, (size_t) st.st_size, PROT_READ, MAP_PRIVATE, file_fd, 0);

	if (*buffer == NULL) {
		log_fatal("Cannot map file to memory!");
		return (size_t) -1;
	}

	close(file_fd);
	free(real_path);

	return (size_t) st.st_size;
}

void userman_destroy() {
	if (sqlite3_close(db) != SQLITE_OK) {
		log_warn("Cannot close database: %s", sqlite3_errmsg(db));
	}

	check_mutex_lu_call(pthread_mutex_unlock(&filedir_mutex));

}
