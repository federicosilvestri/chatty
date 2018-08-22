/*******************************************************************************
 * SOL 2017/2018
 * Chatty
 * Federico Silvestri 559014
 * Si dichiara che il contenuto di questo file e' in ogni sua parte opera
 * originale dell'autore.
 *******************************************************************************/

/**
 * @file signal_manager.h contains signal manager definitions.
 */

#ifndef SIGNAL_MANAGER_H_
#define SIGNAL_MANAGER_H_

#include <stdbool.h>

/*
 * Make Eclipse Happy
 */
#ifndef SIG_BLOCK
/**
 * Signal block definition
 */
#define SIG_BLOCK 1
#endif

/**
 * This function registers a signal as blocked signals.
 * If any errors occurs function will print it.
 *
 * @return false in case of error
 */
bool signal_manager_register();

/**
 * This function put current thread in wait status until a signal is received.
 * If any error occurs function returns immediately.
 *
 * @return the received signal
 */
int signal_manager_wait();

/**
 * This function destroy and cleanup environment for signal manager.
 */
void signal_manager_destroy();

#endif /* SIGNAL_MANAGER_H_ */
