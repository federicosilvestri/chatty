/*******************************************************************************
 * SOL 2017/2018
 * Chatty
 * Federico Silvestri 559014
 * Si dichiara che il contenuto di questo file e' in ogni sua parte opera
 * originale dell'autore.
 *******************************************************************************/

#ifndef LOG_H
#define LOG_H

#include <stdio.h>
#include <stdarg.h>
#include <stdbool.h>

/**
 * LOG level types.
 */
enum { LOG_TRACE, LOG_DEBUG, LOG_INFO, LOG_WARN, LOG_ERROR, LOG_FATAL };

#define log_trace(...) log_log(LOG_TRACE, __FILE__, __LINE__, __VA_ARGS__)
#define log_debug(...) log_log(LOG_DEBUG, __FILE__, __LINE__, __VA_ARGS__)
#define log_info(...)  log_log(LOG_INFO,  __FILE__, __LINE__, __VA_ARGS__)
#define log_warn(...)  log_log(LOG_WARN,  __FILE__, __LINE__, __VA_ARGS__)
#define log_error(...) log_log(LOG_ERROR, __FILE__, __LINE__, __VA_ARGS__)
#define log_fatal(...) log_log(LOG_FATAL, __FILE__, __LINE__, __VA_ARGS__)

/**
 * Initialize internal library objects.
 */
void log_init(int level);

/**
 * Setup a file point where write logs.
 *
 * @param fp file pointer
 */
void log_set_fp(FILE *fp);

/**
 * Set the level you want to log.
 * @param level a valid log level {LOG_TRACE, LOG_DEBUG, ...}
 */
void log_set_level(int level);

/**
 * Enable quiet mode.
 *
 * @param enable true or false
 */
void log_set_quiet(bool enable);

/**
 * Log a string.
 *
 * @param level level of the log
 * @param file file to log
 * @param line line to log
 * @param fmt parameters to pass to printf
 */
void log_log(int level, const char *file, int line, const char *fmt, ...);

/**
 * Delete internal objects and free memory.
 */
void log_destroy();

#endif
