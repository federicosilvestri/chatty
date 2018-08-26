/*******************************************************************************
 * SOL 2017/2018
 * Chatty
 * Federico Silvestri 559014
 * Si dichiara che il contenuto di questo file e' in ogni sua parte opera
 * originale dell'autore.
 *******************************************************************************/
#ifndef CONTROLLER_H
#define CONTROLLER_H

#include <stdbool.h>
#include <libconfig.h>

/**
 * Constant to define that server is running.
 */
#define SERVER_STATUS_RUNNING 0

/**
 * Constant to define that server is stopped.
 */
#define SERVER_STATUS_STOPPED 1

/**
 * Constant to define type of message 'command-request'
 */
#define SERVER_QUEUE_MESSAGE_CMD_REQ 0

/**
 * Constant to define type of message 'write-request'
 */
#define SERVER_QUEUE_MESSAGE_WRITE_REQ 1


/**
 * Configuration variable are available externally.
 */
extern config_t server_conf;

/**
 * Initialize and check if server can be started or not.
 * For example if the current AF_UNIX socket file exists,
 * the result of this function will be false.
 *
 * @return true if yes, false if no
 */
bool server_init();

/**
 * This function starts the chatty server, using all
 * parameters given in server_conf.
 *
 * @brief startup consumer and producer component
 * @return true on success, false on error
 */
bool server_start();

/**
 * This function let returns the status of the server.
 *
 * @return a SERVER_STATUS_* constant
 */
int server_status();

/**
 * Stop the chatty server using a graceful shutdown.
 * This function returns immediatly, because
 * it starts a thread to execute commands
 * in order.
 * @param force set true if you want to force the shutdown
 * @return true on success, false on error
 */
bool server_stop();

/**
 * Destroy the server.
 */
void server_destroy();

/* UTILITY FUNCTIONS */

/**
 * This function checks the result of
 * mutex lock or unlock call.
 * If an error occurs, system will log
 * a fatal exception and will exit(1);
 *
 * @param mutex_res the result of mutex call
 */
void check_mutex_lu_call(int mutex_res);

#endif /* CONTROLLER_H */
