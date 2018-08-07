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

/**
 * to avoid warnings like "ISO C forbids an empty translation unit"
 */
typedef int make_iso_compilers_happy;

#include <stdbool.h>

bool check_arguments(int, char**);
bool parse_config(char*);
void clean_workspace();

#endif //PROJECT_CHATTY_H
