/*
 * membox Progetto del corso di LSO 2017/2018
 *
 * Dipartimento di Informatica Università di Pisa
 * Docenti: Prencipe, Torquati
 * 
 */
/**
 * @file chatty.c
 * @brief File principale del server chatterbox
 */
#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <signal.h>
#include <pthread.h>

/* inserire gli altri include che servono */
#include "includes/chatty.h"
#include "includes/log.h"
#include "stats.h"
#include <stdbool.h>

struct statistics chattyStats = { 0,0,0,0,0,0,0 };


static void usage(const char *progname) {
    fprintf(stderr, "Il server va lanciato con il seguente comando:\n");
    fprintf(stderr, "  %s -f <configuration-file>\n", progname);
}

int main(int argc, char *argv[]) {
    if (check_arguments(argc, argv) == false) {
        return 1;
    }

    log_info("Parameters test passed.");

    return 0;
}

/**
 * This function checks if argument passed by system are valid or not.
 *
 * @param argc system argument count
 * @param argv system argument
 * @return true if arguments are valid, false otherwise
 */
static inline bool check_arguments(int argc, char *argv[]) {
    if (argc < 3) {
        usage(argv[0]);
        return false;
    }

    if (strcmp(argv[1], "-f") != 0) {
        usage(argv[0]);
        return false;
    }

    return true;
}