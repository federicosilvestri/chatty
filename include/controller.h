/*******************************************************************************
 * SOL 2017/2018
 * Chatty
 * Federico Silvestri 559014
 * Si dichiara che il contenuto di questo file e' in ogni sua parte opera
 * originale dell'autore.
 *******************************************************************************/

#ifndef SERVER_H_
#define SERVER_H_

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
 * Constant to define that server is starting.
 */
#define SERVER_STATUS_STOPPING 2

/**
 * Configuration variable are available externally.
 */
extern config_t server_conf;

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
 *
 * @return true on success, false on error
 */
bool server_stop();

/**
 * This function puts the controller thread in wait status.
 */
void server_join();

#endif /* SERVER_H_ */
