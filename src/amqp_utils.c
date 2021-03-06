/*******************************************************************************
 * SOL 2017/2018
 * Chatty
 * Federico Silvestri 559014
 * Si dichiara che il contenuto di questo file e' in ogni sua parte opera
 * originale dell'autore.
 *******************************************************************************/
/**
 * @brief This file is a simple manager of RabbitMQ server.
 * @file amqp_utils.c
 */

/**
 * C POSIX source definition.
 */
#define _POSIX_C_SOURCE 200809L

#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libconfig.h>
#include <stdint.h>
#include <amqp.h>
#include <amqp_tcp_socket.h>

#include "log.h"
#include "amqp_utils.h"

/**
 * Status of library
 */
static bool initialized;

/**
 * External configuration from libconfig
 */
extern config_t server_conf;

/**
 * Hostname of RabbitMQ server
 */
static const char *rabmq_hostname;

/**
 * Port of RabbitMQ server
 */
static int rabmq_port;

/**
 * The name of RabbitMQ exchange
 */
const char *rabmq_exchange;

/**
 * Bind key of RabbitMQ
 */
const char *rabmq_bindkey;

bool rabmq_init_params() {
	// fetching parameters from master
	if (config_lookup_string(&server_conf, "RabbitMQHostname",
			&rabmq_hostname) == CONFIG_FALSE) {
		log_error("Cannot get RabbitMQHostname parameter");
		return false;
	}

	if (config_lookup_int(&server_conf, "RabbitMQPort",
			&rabmq_port) == CONFIG_FALSE) {
		log_error("Cannot get RabbitMQPort parameter");
		return false;
	}

	if (config_lookup_string(&server_conf, "RabbitMQExchange",
			&rabmq_exchange) == CONFIG_FALSE) {
		log_error(
				"Cannot get RabbitMQExchange. Check if default value is valid.");
		return false;

	}

	if (config_lookup_string(&server_conf, "RabbitMQBindKey",
			&rabmq_bindkey) == CONFIG_FALSE) {
		log_error(
				"Cannot get RabbitMQBindKey. Check if default value is valid.");
		return false;

	}

	log_debug("Loaded params for RMQ: [ %s, %d, %s, %s] ", rabmq_hostname,
			rabmq_port, rabmq_exchange, rabmq_bindkey);

	initialized = true;
	return true;
}

bool rabmq_declare_init() {
	amqp_socket_t *socket = NULL;
	amqp_connection_state_t conn;

	if (!rabmq_init(&socket, &conn)) {
		log_fatal("Cannot connect to RabbitMQ server during queue declaration");
		return false;
	}

	amqp_channel_open(conn, 1);
	amqp_check_error(amqp_get_rpc_reply(conn), "Opening channel");

	log_debug("Declaring exchange");
	amqp_exchange_declare_ok_t *ex_reply = amqp_exchange_declare(conn, 1,
			amqp_cstring_bytes(rabmq_exchange),
			amqp_cstring_bytes("fanout"), 0, 0, 0, 0, amqp_empty_table);
	if (ex_reply == NULL) {
		amqp_check_error(amqp_get_rpc_reply(conn), "Exchange declaring");
		return false;
	}

	log_debug("Declaring queue");
	amqp_queue_declare_ok_t *q_reply = amqp_queue_declare(conn, 1,
			amqp_cstring_bytes(RABBIT_QUEUE_NAME), 0, 0, 0, 1,
			amqp_empty_table);
	if (q_reply == NULL) {
		amqp_check_error(amqp_get_rpc_reply(conn), "Queue declaring");
		return false;
	}

	log_debug("Binding queue...");
	amqp_queue_bind(conn, 1, amqp_cstring_bytes(RABBIT_QUEUE_NAME),
			amqp_cstring_bytes(rabmq_exchange),
			amqp_cstring_bytes(rabmq_bindkey), amqp_empty_table);
	amqp_check_error(amqp_get_rpc_reply(conn), "Binding queue");

	amqp_check_error(amqp_channel_close(conn, 1, AMQP_REPLY_SUCCESS),
			"Closing declaring queue channel");
	rabmq_destroy(&conn);

	return true;
}

bool rabmq_init(amqp_socket_t **socket, amqp_connection_state_t *conn) {
	if (!initialized) {
		log_fatal("RabbitMQ configuration parameters are not initialized.");
		return false;
	}

	*conn = amqp_new_connection();

	*socket = amqp_tcp_socket_new(*conn);
	if (!*socket) {
		// destroy socket
		amqp_destroy_connection(*conn);
		log_error("creating TCP socket");
		return false;
	}

	int p_status = amqp_socket_open(*socket, rabmq_hostname, rabmq_port);
	if (p_status) {
		amqp_destroy_connection(*conn);
		log_error("opening TCP socket");
		return false;
	}

	bool conn_error = amqp_check_error(
			amqp_login(*conn, "/", 0, 131072, 0, AMQP_SASL_METHOD_PLAIN,
					"guest", "guest"), "Can't connect to RabbitMQ");

	if (conn_error) {
		amqp_destroy_connection(*conn);
		log_error("Cannot start server due to previous error");
		return false;
	}

	return true;
}

void rabmq_destroy(amqp_connection_state_t *conn) {
	if (conn == NULL) {
		return;
	}

	// close socket to rabbit
	log_debug("Closing RabbitMQ connection");

	amqp_check_error(amqp_connection_close(*conn, AMQP_REPLY_SUCCESS),
			"Cannot close connection to RabbitM");

	// try to destroy (also if it isn't closed)

	log_debug("Destroying RabbitMQ connection");
	int c_destroy = amqp_destroy_connection(*conn);

	if (c_destroy < 0) {
		log_error("Cannot destroy connection to RabbitMQ, %s",
				amqp_error_string2(c_destroy));
	}
}

bool amqp_check_error(amqp_rpc_reply_t x, char const *context) {
	switch (x.reply_type) {
	case AMQP_RESPONSE_NORMAL:
		return false;

	case AMQP_RESPONSE_NONE:
		log_error("%s: missing RPC reply type!\n", context);
		break;

	case AMQP_RESPONSE_LIBRARY_EXCEPTION:
		log_error("%s: %s\n", context, amqp_error_string2(x.library_error));
		break;

	case AMQP_RESPONSE_SERVER_EXCEPTION:
		switch (x.reply.id) {
		case AMQP_CONNECTION_CLOSE_METHOD: {
			amqp_connection_close_t *m =
					(amqp_connection_close_t *) x.reply.decoded;
			log_error("%s: server connection error %uh, message: %.*s\n",
					context, m->reply_code, (int )m->reply_text.len,
					(char * )m->reply_text.bytes);
			break;
		}
		case AMQP_CHANNEL_CLOSE_METHOD: {
			amqp_channel_close_t *m = (amqp_channel_close_t *) x.reply.decoded;
			log_error("%s: server channel error %uh, message: %.*s\n", context,
					m->reply_code, (int )m->reply_text.len,
					(char * )m->reply_text.bytes);
			break;
		}
		default:
			log_error("%s: unknown server error, method id 0x%08X\n", context,
					x.reply.id);
			break;
		}
		break;
	}

	return true;
}

static void dump_row(long count, int numinrow, int *chs) {
	int i;

	printf("%08lX:", count - numinrow);

	if (numinrow > 0) {
		for (i = 0; i < numinrow; i++) {
			if (i == 8) {
				printf(" :");
			}
			printf(" %02X", chs[i]);
		}
		for (i = numinrow; i < 16; i++) {
			if (i == 8) {
				printf(" :");
			}
			printf("   ");
		}
		printf("  ");
		for (i = 0; i < numinrow; i++) {
			if (isprint(chs[i])) {
				printf("%c", chs[i]);
			} else {
				printf(".");
			}
		}
	}
	printf("\n");
}

static int rows_eq(int *a, int *b) {
	int i;

	for (i = 0; i < 16; i++)
		if (a[i] != b[i]) {
			return 0;
		}

	return 1;
}

void amqp_dump(void const *buffer, size_t len) {
	unsigned char *buf = (unsigned char *) buffer;
	long count = 0;
	int numinrow = 0;
	int chs[16];
	int oldchs[16] = { 0 };
	int showed_dots = 0;
	size_t i;

	for (i = 0; i < len; i++) {
		int ch = buf[i];

		if (numinrow == 16) {
			int j;

			if (rows_eq(oldchs, chs)) {
				if (!showed_dots) {
					showed_dots = 1;
					printf(
							"          .. .. .. .. .. .. .. .. : .. .. .. .. .. .. .. ..\n");
				}
			} else {
				showed_dots = 0;
				dump_row(count, numinrow, chs);
			}

			for (j = 0; j < 16; j++) {
				oldchs[j] = chs[j];
			}

			numinrow = 0;
		}

		count++;
		chs[numinrow++] = ch;
	}

	dump_row(count, numinrow, chs);

	if (numinrow != 0) {
		printf("%08lX:\n", count);
	}
}
