/*
 * server.c
 *
 *  Created on: Aug 4, 2018
 *      Author: federicosilvestri
 */

#include "server.h"

#include <stdio.h>
#include <stdlib.h>

#include <amqp.h>
#include <amqp_tcp_socket.h>

#include "config.h"
#include "log.h"

const char *rabmq_hostname;
int rabmq_port;

amqp_socket_t *p_socket = NULL;
amqp_connection_state_t p_conn;
int p_status;
//static int p_message_count = 10;
//static int p_rate_limit = 20;

bool rabmq_init_params() {
	// fetching parameters from master
	if (config_lookup_string(&server_conf, "RabbitMQHostname", &rabmq_hostname)
			!= 0) {
		log_error("Cannot get RabbitMQHostname parameter");
		return false;
	}

	if (config_lookup_int(&server_conf, "RabbitMQPort", &rabmq_port) != 0) {
		log_error("Cannot get RabbitMQPort parameter");
		return false;
	}

	return false;
}

bool producer_init() {
	// connect to rabbit
	log_debug("Creating production amqp connection");

//	p_conn = amqp_new_connection();
//
//	p_socket = amqp_tcp_socket_new(p_conn);
//	if (!p_socket) {
//		log_error("creating TCP socket");
//		return false;
//	}
//
//	p_status = amqp_socket_open(p_socket, rabmq_hostname, rabmq_port);
//	if (p_status) {
//		log_error("opening TCP socket");
//		return false;
//	}
//
//	amqp_login(p_conn, "/", 0, 131072, 0, AMQP_SASL_METHOD_PLAIN, "guest",
//			"guest");

//	amqp_channel_open(p_conn, 1);
//	die_on_amqp_error(amqp_get_rpc_reply(p_conn), "Opening channel");
//
//	send_batch(p_conn, "test queue", p_rate_limit, p_message_count);
//
//	die_on_amqp_error(amqp_channel_close(p_conn, 1, AMQP_REPLY_SUCCESS),
//			"Closing channel");

	return true;

}

bool producer_destroy() {
	// close socket to rabbit
//	amqp_connection_close(p_conn, AMQP_REPLY_SUCCESS);
//	amqp_destroy_connection(p_conn);

	return true;
}
