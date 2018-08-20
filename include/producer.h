/*******************************************************************************
 * SOL 2017/2018
 * Chatty
 * Federico Silvestri 559014
 * Si dichiara che il contenuto di questo file e' in ogni sua parte opera
 * originale dell'autore.
 *******************************************************************************/
#ifndef PRODUCER_H
#define PRODUCER_H

#include <stdbool.h>
#include <libconfig.h>

extern struct config_t server_conf;

/**
 * @brief initializes all workspace for producer
 * @return true on success, false on error
 */
bool producer_init();

/**
 * Start the producer thread.
 *
 * @return true on success, false on error
 */
bool producer_start();

/**
 * Wait termination of thread.
 */
void producer_wait();

/**
 * Destroy the producer
 */
void producer_destroy();

/**
 * This function disconnects the select host
 * from server.
 * It closes the file descriptor of socket
 * and unlocks the socket block.
 * Operation on selected socket after the call
 * of this functions will generate a fault.
 * @brief disconnect the host
 * @param index of socket array to disconnect
 */
void producer_disconnect_host(int);

/**
 * This function unlocks (releases) the selected
 * socket to producer, and it must be used
 * when a worker ends the execution.
 * @brief release socket to producer
 * @param int socket index
 */
void producer_unlock_socket(int);

#endif /* PRODUCER_H */
