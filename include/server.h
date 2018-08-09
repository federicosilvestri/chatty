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

extern config_t server_conf;

bool server_start();
bool rabmq_init_params();
bool producer_init();
bool producer_destroy();

#endif /* SERVER_H_ */
