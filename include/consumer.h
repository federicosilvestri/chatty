/*******************************************************************************
 * SOL 2017/2018
 * Chatty
 * Federico Silvestri 559014
 * Si dichiara che il contenuto di questo file e' in ogni sua parte opera
 * originale dell'autore.
 *******************************************************************************/
#ifndef CONSUMER_H_
#define CONSUMER_H_

#include <stdbool.h>

/**
 * This function initializes the consumer thread pool.
 * In case of ThreadsInPool parameters missing, the function
 * returns false.
 * @return false in case of initialization failed, true on success
 */
bool consumer_init();

/**
 * This function starts up the consumer thread pool.
 */
bool consumer_start();

/**
 * Waits termination of thread pool.
 */
void consumer_wait();

/**
 * This function destroys the consumer service.
 */
void consumer_destroy();


#endif /* CONSUMER_H_ */
