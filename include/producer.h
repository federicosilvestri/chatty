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

/*
 * Make Eclipse Code Control happy
 */
#ifndef SIGBLOCK
#define SIGBLOCK 1
#endif

/**
 * External configuration
 * by libconfig.
 */
extern config_t server_conf;

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
 * Stop and wait termination of thread.
 */
void producer_stop();

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
void producer_disconnect_host(int index);

/**
 * This function locks (block) the selected
 * socket to producer, and it must be used
 * when you want to use the socket.
 * @brief lock socket to producer
 * @param index socket index
 */
void producer_lock_socket(int index);

/**
 * This function unlocks (releases) the selected
 * socket to producer, and it must be used
 * when a worker ends the execution.
 * @brief release socket to producer
 * @param index socket index
 */
void producer_unlock_socket(int index);

/**
 * This function set the connected user to the current fd.
 * It is useful to disconnect dead clients.
 *
 * @param index of socket
 * @param nickname of socket, NULL if you want to disconnect
 */
void producer_set_fd_nickname(int index, char* nickname);

/**
 * This function copy the nickname associated to fd
 * in the passed string pointer.
 * Remember to free pointer after initialization.
 *
 * @param index socket index
 * @param nickname pointer to string
 */
void producer_get_fd_nickname(int index, char** nickname);

/**
 * This function retrieve the socket where user with nickname is connected.
 * @param nickname to search
 * @return -1 in case of fail, else the fd
 */
int producer_get_fd_by_nickname(char* nickname);

#endif /* PRODUCER_H */
