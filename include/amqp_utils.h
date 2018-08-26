/*******************************************************************************
 * SOL 2017/2018
 * Chatty
 * Federico Silvestri 559014
 * Si dichiara che il contenuto di questo file e' in ogni sua parte opera
 * originale dell'autore.
 *******************************************************************************/

#ifndef AMQP_UTILS_H_
#define AMQP_UTILS_H_

#include <stdbool.h>
#include <amqp.h>

#define RABBIT_QUEUE_NAME "chatty-queue"

/**
 * This function retrieves RabbitMQ parameters by server_conf
 * variable and store information on static variables.
 *
 * @brief rabbit parameters initialization
 * @return true on success, false on error
 */
bool rabmq_init_params();

/**
 * Initialize RabbitMQ connection state and socket.
 *
 * @param socket socket of rabbit
 * @param conn connection of rabbit
 * @return true on success, false on error
 */
bool rabmq_init(amqp_socket_t **socket, amqp_connection_state_t *conn);

/**
 * Declare queue on RabbitMQ.
 *
 * @return true on success false on error
 */
bool rabmq_declare_init();

/**
 * This function checks if a specific family of rabbitmq-lib
 * functions return an error or not.
 *
 * @param x reply of function
 * @param context context to print in case of error
 * @return false if functions return does not throw an error, true otherwise
 */
bool amqp_check_error(amqp_rpc_reply_t x, char const *context);

/**
 * Destroy connection and socket to RabbitMQ.
 *
 * @param conn connection to destroy
 */
void rabmq_destroy(amqp_connection_state_t *conn);

/**
 * It dumps useful information about queue.
 * @param buffer buffer of queue
 * @param len length of buffer
 */
void amqp_dump(void const *buffer, size_t len);

#endif /* AMQP_UTILS_H_ */
