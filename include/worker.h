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

void worker_run(amqp_message_t);



#endif /* WORKER_H_ */
