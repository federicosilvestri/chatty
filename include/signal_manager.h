/*******************************************************************************
 * SOL 2017/2018
 * Chatty
 * Federico Silvestri 559014
 * Si dichiara che il contenuto di questo file e' in ogni sua parte opera
 * originale dell'autore.
 *******************************************************************************/

#ifndef SIGNAL_HANDLER_H_
#define SIGNAL_HANDLER_H_

#include <stdbool.h>

/**
 * This function registers a signal as blocked signals.
 * @example if you register SIGINT, it will be ignored if received but managed by wait_signal.
 *
 * If any errors occurs function will print it.
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

#endif /* SIGNAL_HANDLER_H_ */
