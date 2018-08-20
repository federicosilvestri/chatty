/*******************************************************************************
 * SOL 2017/2018
 * Chatty
 * Federico Silvestri 559014
 * Si dichiara che il contenuto di questo file e' in ogni sua parte opera
 * originale dell'autore.
 *******************************************************************************/
#ifndef USERMAN_H

#include <stdbool.h>

#include "message.h"

/**
 * A very primitive data structure
 * for user.
 */
typedef struct {
	char *username;
	long last_login;
} userman_user_t;

/**
 * Initializes userman.
 * @return true on success, false on error.
 */
bool userman_init();

/**
 * Deallocate and destroy userman.
 */
void userman_destroy();


#define USERMAN_H
#endif /* USERMAN_H */
