/*******************************************************************************
 * SOL 2017/2018
 * Chatty
 * Federico Silvestri 559014
 * Si dichiara che il contenuto di questo file e' in ogni sua parte opera
 * originale dell'autore.
 *******************************************************************************/
#ifndef WORKER_H_
#define WORKER_H_

#include <stdbool.h>
#include <amqp.h>


/**
 * Initializes the Worker, reading config
 * from libconfig.
 * Do not execute more than one time.
 *
 * @return true on success, false on error
 */
bool worker_init();

/**
 * Execute the message management related
 * to a specific socket (passed inside message).
 *
 *
 * @param amqp_message_t message received by RabbitMQ.
 */
void worker_run(amqp_message_t);

#endif /* WORKER_H_ */
