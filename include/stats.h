/*
 * chatterbox Progetto del corso di LSO 2017/2018
 *
 * Dipartimento di Informatica Università di Pisa
 * Docenti: Prencipe, Torquati
 * 
 */

/**
 * @brief header of stats.c
 * @file stats.h
 */

#ifndef CHATTY_STATS
#define CHATTY_STATS

#include <stdio.h>
#include <time.h>
#include <stdbool.h>

/**
 * Statistics structure.
 */
struct statistics {
	/**
	 * Registered users
	 */
	unsigned long nusers;
	/**
	 * Online users
	 */
	unsigned long nonline;
	/**
	 * Number of delivered messages
	 */
	unsigned long ndelivered;
	/**
	 * Number of not delivered messages
	 */
	unsigned long nnotdelivered;
	/**
	 * Number of file delivered
	 */
	unsigned long nfiledelivered;
	/**
	 * Number of file not delivered
	 */
	unsigned long nfilenotdelivered;
	/**
	 * Number of errors
	 */
	unsigned long nerrors;
};

extern struct statistics chattyStats;

/**
 * Macro to update the value of nusers.
 */
#define stats_update_reg_users(...) 	stats_update_value(__VA_ARGS__, &chattyStats.nusers)

/**
 * Macro to update the value of ndelivered
 */
#define stats_update_dev_msgs(...) 	stats_update_value(__VA_ARGS__, &chattyStats.ndelivered)

/**
 * Macro to update the value of nnotdelivered
 */
#define stats_update_ndev_msgs(...) 	stats_update_value(__VA_ARGS__, &chattyStats.nnotdelivered)

/**
 * Macro to update the value of nfiledelivered
 */
#define stats_update_dev_file(...) 	stats_update_value(__VA_ARGS__, &chattyStats.nfiledelivered)

/**
 * Macro to update the value of nfilenotdelivered
 */
#define stats_update_ndev_file(...) 	stats_update_value(__VA_ARGS__, &chattyStats.nfilenotdelivered)

/**
 * Macro to update the value of nerrors
 */
#define stats_update_errors(...)		stats_update_value(__VA_ARGS__, 0,  &chattyStats.nerrors)

/**
 * @function printStats
 * @brief Stampa le statistiche nel file passato come argomento
 *
 * @param fout descrittore del file aperto in append.
 *
 * @return 0 in caso di successo, -1 in caso di fallimento 
 */
static inline int printStats(FILE *fout) {
	extern struct statistics chattyStats;

	if (fprintf(fout, "%ld - %ld %ld %ld %ld %ld %ld %ld\n",
			(unsigned long) time(NULL), chattyStats.nusers, chattyStats.nonline,
			chattyStats.ndelivered, chattyStats.nnotdelivered,
			chattyStats.nfiledelivered, chattyStats.nfilenotdelivered,
			chattyStats.nerrors) < 0)
		return -1;
	fflush(fout);
	return 0;
}

/**
 * Initialize the statistics component
 *
 * @return true on success, false on error
 */
bool stats_init();

/**
 * Send a request for print statistics.
 */
void stats_trigger();

/**
 * DO NOT USE this function if you don't know what you are doing.
 * Update the values of statistics.
 * @param add the value to add to stat
 * @param remove the value to remove
 * @param dest pointer to stat value
 *
 */
void stats_update_value(unsigned long add, unsigned long remove, unsigned long *dest);

/**
 * Destroy the statistics component (it frees memory)
 */
void stats_destroy();

#endif /* CHATTY_STATS */
