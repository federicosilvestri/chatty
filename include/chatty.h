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

//#define LOG_USE_COLOR 1

#include <stdbool.h>
#include <errno.h>
#include "libconfig.h"
#include "stats.h"
#include "log.h"
#include "config.h"

// structure that contains server configuration
config_t server_conf;

static inline bool check_arguments(int, char**);
bool parse_config(char*);

#endif //PROJECT_CHATTY_H
