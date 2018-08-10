/*******************************************************************************
 * SOL 2017/2018
 * Chatty
 * Federico Silvestri 559014
 * Si dichiara che il contenuto di questo file e' in ogni sua parte opera
 * originale dell'autore.
 *******************************************************************************/

#ifndef AMQP_UTILS_H_
#define AMQP_UTILS_H_

#include <stdbool.h>
#include <amqp.h>

/**
 * This function checks if a specific family of rabbitmq-lib
 * functions return an error or not.
 *
 * @param x reply of function
 * @param context context to print in case of error
 * @return false if functions return does not throw an error, true otherwise
 */
extern bool amqp_check_error(amqp_rpc_reply_t x, char const *context);

extern void amqp_dump(void const *buffer, size_t len);
extern uint64_t now_microseconds(void);
extern void microsleep(int usec);

#endif /* AMQP_UTILS_H_ */
