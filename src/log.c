/*******************************************************************************
 * SOL 2017/2018
 * Chatty
 * Federico Silvestri 559014
 * Si dichiara che il contenuto di questo file e' in ogni sua parte opera
 * originale dell'autore.
 *******************************************************************************/
/**
 * Simple library to log file.
 * Not written completely by me, but optimized for POSIX threads
 * and adapted for this project.
 *
 * @brief File to manage simple logging system.
 * @file log.c
 */

/**
 * Define Posix Source
 */
#define _POSIX_C_SOURCE 200809L

#include "log.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <pthread.h>
#include <time.h>

static bool initialized = false;

static struct {
	pthread_mutex_t lock;
	FILE *fp;
	int level;
	bool quiet;
} L;

static const char *level_names[] = { "TRACE", "DEBUG", "INFO", "WARN", "ERROR",
		"FATAL" };

static const char *level_colors[] = { "\x1b[94m", "\x1b[36m", "\x1b[32m",
		"\x1b[33m", "\x1b[31m", "\x1b[35m" };

void log_init(int level) {
	if (initialized == true) {
		return;
	}
	initialized = true;

	pthread_mutex_init(&L.lock, NULL);
	if (level == 6) {
		log_set_quiet(true);
	} else {
		log_set_level(level);
	}
}

void log_set_fp(FILE *fp) {
	L.fp = fp;
}

void log_set_level(int level) {
	L.level = level;
}

void log_set_quiet(bool enable) {
	L.quiet = enable;
}

void log_log(int level, const char *file, int line, const char *fmt, ...) {
	if (level < L.level || initialized == false) {
		return;
	}

	// Acquire lock
	if (pthread_mutex_lock(&L.lock) < 0) {
		fprintf(stderr, "LOG Library cannot lock mutex");
		fflush(stderr);
		exit(1);
	}

	// Get current time
	time_t t = time(NULL);
	struct tm *lt = localtime(&t);

	// Log to sterr
	if (!L.quiet) {
		va_list args;
		char buf[16];
		buf[strftime(buf, sizeof(buf), "%H:%M:%S", lt)] = '\0';
		fprintf(
		stderr, "%s %s%-5s\x1b[0m \x1b[90m%s:%d:\x1b[0m ", buf,
				level_colors[level], level_names[level], file, line);
		va_start(args, fmt);
		vfprintf(stderr, fmt, args);
		va_end(args);
		fprintf(stderr, "\n");
		fflush(stderr);
	}

	// Log to file
	if (L.fp) {
		va_list args;
		char buf[32];
		buf[strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", lt)] = '\0';
		fprintf(L.fp, "%s %-5s %s:%d: ", buf, level_names[level], file, line);
		va_start(args, fmt);
		vfprintf(L.fp, fmt, args);
		va_end(args);
		fprintf(L.fp, "\n");
		fflush(L.fp);
	}

	// Release lock
	if (pthread_mutex_unlock(&L.lock) < 0) {
		fprintf(stderr, "LOG Library cannot unlock mutex");
		fflush(stderr);
		exit(1);
	}
}

void log_destroy() {
	pthread_mutex_destroy(&L.lock);
}
