/*******************************************************************************
 * SOL 2017/2018
 * Chatty
 * Federico Silvestri 559014
 * Si dichiara che il contenuto di questo file e' in ogni sua parte opera
 * originale dell'autore.
 *******************************************************************************/
#ifndef PROJECT_CHATTY_H
#define PROJECT_CHATTY_H

/**
 * to avoid warnings like "ISO C forbids an empty translation unit"
 */
typedef int make_iso_compilers_happy;

#ifndef STD_LOG_LEVEL
#define STD_LOG_LEVEL 1
#endif

#include <stdbool.h>

static bool checkandget_arguments(int, char**, int*);

static void clean_workspace();

#endif //PROJECT_CHATTY_H
