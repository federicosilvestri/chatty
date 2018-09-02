/*******************************************************************************
 * SOL 2017/2018
 * Chatty
 * Federico Silvestri 559014
 * Si dichiara che il contenuto di questo file e' in ogni sua parte opera
 * originale dell'autore.
 *******************************************************************************/
/**
 * @brief header of chatty.c
 * @file chatty.h
 */

#ifndef PROJECT_CHATTY_H
#define PROJECT_CHATTY_H

/**
 * to avoid warnings like "ISO C forbids an empty translation unit"
 */
typedef int make_iso_compilers_happy;

#ifndef STD_LOG_LEVEL
/**
 * The standard log level to use when log level argument is not passed.
 */
#define STD_LOG_LEVEL 4
#endif

#include <stdbool.h>

/**
 * Check and get the arguments passed to program.
 *
 * @param argc the argument count
 * @param argv an array of string that contains arguments
 * @param the used log level for runtime
 * @return true if configuration parsing is ok, false in other cases.
 */
static bool checkandget_arguments(int argc, char *argv[], int *log_level);

/**
 * Cleanup the workspace
 * used by chatty
 */
static void clean_workspace();

#endif //PROJECT_CHATTY_H
