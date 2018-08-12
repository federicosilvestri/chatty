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

#include "controller.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>

#include <amqp.h>
#include <amqp_tcp_socket.h>

#include "config.h"
#include "log.h"
#include "amqp_utils.h"
#include "producer.h"

const char *rabmq_hostname;
const char *rabmq_exchange;
const char *rabmq_bindkey;
int rabmq_port;

amqp_socket_t *p_socket = NULL;
amqp_connection_state_t p_conn;
int p_status;

int *sockets;

/**
 * This is the internal status variable.
 * It must not be affected by concurrency race.
 */
static int status = SERVER_STATUS_STOPPED;

/**
 * This function retrieves RabbitMQ parameters by server_conf
 * variable and store information on static variables.
 *
 * @brief rabbit parameters initialization
 * @return true on success, false on error
 */
static bool rabmq_init_params() {
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

/**
 * It initialize connection to RabbitMQ server with
 * fetched parameters.
 *
 * @brief create connection to RabbitMQ server
 * @return true on success, false on error
 */
static bool rabmq_init() {
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

	bool conn_error = amqp_check_error(
			amqp_login(p_conn, "/", 0, 131072, 0, AMQP_SASL_METHOD_PLAIN,
					"guest", "guest"), "Can't connect to RabbitMQ");

	if (conn_error) {
		log_error("Cannot start server due to previous error");
		return false;
	}

	return true;
}

bool server_start() {
	if (status == SERVER_STATUS_STOPPING || status == SERVER_STATUS_RUNNING) {
		log_error("server_start call received multiple times!");
		return false;
	}

	if (rabmq_init_params() == false) {
		return false;
	}

	if (!rabmq_init()) {
		return false;
	}

	if (producer_init() == false) {
		producer_destroy();
		return false;
	}

	// consumer init
	// consumer start

	if (producer_start() == false) {
		producer_destroy();
		return false;
	}

	// update status
	status = SERVER_STATUS_RUNNING;

	return true;
}

static void rabbitmq_destroy() {
	// close socket to rabbit
	log_debug("Closing RabbitMQ connection");

	amqp_check_error(amqp_connection_close(p_conn, AMQP_REPLY_SUCCESS),
			"Cannot close connection to RabbitM");

	// try to destroy (also if it isn't closed)

	log_debug("Destroying RabbitMQ connection");
	int c_destroy = amqp_destroy_connection(p_conn);

	if (c_destroy < 0) {
		log_error("Cannot destroy connection to RabbitMQ, %s",
				amqp_error_string2(c_destroy));
	}
}

bool server_stop() {
	log_trace("server_stop execution");

	if (status == SERVER_STATUS_STOPPING) {
		log_warn("server_stop call received multiple times!");
		return false;
	}

	// update status
	status = SERVER_STATUS_STOPPED;

	log_trace("server_stop executed.");

	return true;
}

int server_status() {
	return status;
}

void server_wait() {
	// wait termination of consumer thread
	producer_wait();
	// consumer_join();
}

void server_destroy() {
	// consumer_destroy();
	rabbitmq_destroy();

	// producer
	producer_destroy();

	// consumer
	// consumer_destroy();
}
