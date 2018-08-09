/*******************************************************************************
 * SOL 2017/2018
 * Chatty
 * Federico Silvestri 559014
 * Si dichiara che il contenuto di questo file e' in ogni sua parte opera
 * originale dell'autore.
 *******************************************************************************/

#include "server.h"

#include <stdio.h>
#include <stdlib.h>
#include <memory.h>

#include <amqp.h>
#include <amqp_tcp_socket.h>

#include "config.h"
#include "log.h"
#include "amqp_utils.h"

const char *rabmq_hostname;
const char *rabmq_exchange;
const char *rabmq_bindkey;
int rabmq_port;

amqp_socket_t *p_socket = NULL;
amqp_connection_state_t p_conn;
int p_status;
//static int p_message_count = 10;
//static int p_rate_limit = 20;

bool server_start() {
	if (rabmq_init_params() == false) {
		return false;
	}

	if (producer_init() == false) {
		return false;
	}

	producer_destroy();

	return true;
}

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

	return true;
}

bool producer_init() {
	// connect to rabbit
	log_debug("Creating production amqp connection");

	p_conn = amqp_new_connection();

	p_socket = amqp_tcp_socket_new(p_conn);
	if (!p_socket) {
		log_error("creating TCP socket");
		return false;
	}

	p_status = amqp_socket_open(p_socket, rabmq_hostname, rabmq_port);
	if (p_status) {
		log_error("opening TCP socket");
		return false;
	}

	amqp_login(p_conn, "/", 0, 131072, 0, AMQP_SASL_METHOD_PLAIN, "guest",
			"guest");

	// open socke

	return true;

}

void producer_idle() {
	//	amqp_channel_open(p_conn, 1);
	//	die_on_amqp_error(amqp_get_rpc_reply(p_conn), "Opening channel");
	//
	//	send_batch(p_conn, "test queue", p_rate_limit, p_message_count);
	//
	//	die_on_amqp_error(amqp_channel_close(p_conn, 1, AMQP_REPLY_SUCCESS),
	//			"Closing channel");
}

bool producer_destroy() {
	// close socket to rabbit
	amqp_check_error(amqp_connection_close(p_conn, AMQP_REPLY_SUCCESS),
			"Cannot close connection to RabbitM");

	// try to destroy (also if it isn't closed)

	int c_destroy = amqp_destroy_connection(p_conn);

	if (c_destroy < 0) {
		log_error("Cannot destroy connection to RabbitMQ, %s", amqp_error_string2(c_destroy));
	}

	return true;
}
