/*
 * chatterbox Progetto del corso di LSO 2017/2018
 *
 * Dipartimento di Informatica Università di Pisa
 * Docenti: Prencipe, Torquati
 * 
 */
#if !defined(MEMBOX_STATS_)
#define MEMBOX_STATS_

#include <stdio.h>
#include <time.h>

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

/* aggiungere qui altre funzioni di utilita' per le statistiche */

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

#endif /* MEMBOX_STATS_ */
