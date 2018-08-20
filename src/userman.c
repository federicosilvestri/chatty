/*******************************************************************************
 * SOL 2017/2018
 * Chatty
 * Federico Silvestri 559014
 * Si dichiara che il contenuto di questo file e' in ogni sua parte opera
 * originale dell'autore.
 *******************************************************************************/
/**
 * C POSIX source definition.
 */
#define _POSIX_C_SOURCE 200809L

#include "userman.h"

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <libconfig.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/signal.h>
#include <amqp.h>
#include <amqp_tcp_socket.h>

#include "log.h"
#include "amqp_utils.h"
#include "controller.h"
#include "worker.h"

bool userman_init() {
	static bool init = false;

	if (init) {
		log_fatal("Userman initialized multiple times");
		return false;
	}

	// connect to sqlite (?)

	return true;
}
