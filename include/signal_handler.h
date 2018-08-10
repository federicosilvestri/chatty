/*******************************************************************************
 * SOL 2017/2018
 * Chatty
 * Federico Silvestri 559014
 * Si dichiara che il contenuto di questo file e' in ogni sua parte opera
 * originale dell'autore.
 *******************************************************************************/

#ifndef SIGNAL_HANDLER_H_
#define SIGNAL_HANDLER_H_

/**
 * C POSIX source definition.
 */
#define _POSIX_C_SOURCE 200809L

#include <stdbool.h>
#include <signal.h>

/**
 * How many signals are registered.
 */
#define SIG_HANDL_N 2

/**
 * SIGPIPE signal code
 */
#define SIG_HANDL_SIGPIPE 0

/**
 * SIGTERM signal code
 */
#define SIG_HANDL_SIGINT 1

/**
 * SIGUSR1 signal code
 */
#define SIG_HANDL_SIGUSR1 2

/**
 * Registered signal to register to system
 */
extern const int sig_handl_signals[];

/**
 * Actions to perform when signal is triggered.
 */
extern struct sigaction sig_handl_acts[];

/**
 * @brief initialize signal handler
 */
void signal_handler_init();

/**
 * This function registers all handlers for
 * program.
 * If any errors occurs function will print it.
 * @return false in case of error
 */
bool signal_handler_register();

/**
 * Handler for SIGPIPE
 * @brief this function manages the SIGPIPE signal.
 */
void signal_handler_pipe();

/**
 * Handler for SIGINT
 * @brief this function manages the SIGINT signal.
 */
void signal_handler_int();

/**
 * Handler for SIGTERM.
 * @brief this function manages the operation for stopping server.
 */
void signal_handler_term();

/**
 * Handler for SIGUSR1.
 * @brief this function manages the statistic printing trigger.
 */
void signal_handler_usr1();

#endif /* SIGNAL_HANDLER_H_ */
