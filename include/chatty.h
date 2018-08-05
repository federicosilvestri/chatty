/*
 * SOL 2017/2018
 * Chatty
 *
 * \author Federico Silvestri 559014
 *
 * Si dichiara che il contenuto di questo file e' in ogni sua parte opera
 * originale dell'autore.
 *
*/

#ifndef PROJECT_CHATTY_H
#define PROJECT_CHATTY_H

#include <stdbool.h>
#include <errno.h>
#include <signal.h>
#include <libconfig.h>

#include "log.h"
#include "stats.h"
#include "config.h"
#include "signal_handler.h"

// structure that contains server configuration
config_t server_conf;

bool check_arguments(int, char**);
bool parse_config(char*);
void clean_workspace();

#endif //PROJECT_CHATTY_H
