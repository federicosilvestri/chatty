/*
 * chatterbox Progetto del corso di LSO 2017/2018
 *
 * Dipartimento di Informatica Università di Pisa
 * Docenti: Prencipe, Torquati
 * 
 */
#ifndef MESSAGE_H_
#define MESSAGE_H_

#include <assert.h>
#include <string.h>
#include "ops.h"

/**
 * Max length of user name
 */
#define MAX_NAME_LENGTH	120

/**
 * @file  message.h
 * @brief Contiene il formato del messaggio
 */

/**
 *  @struct message_hdr_t
 *  @brief header del messaggio 
 *
 */
typedef struct {
	/**
	 * Sipo di connessione richiesta al server
	 */
	op_t op;
	/**
	 * Sender nickname del mittente
	 */
	char sender[MAX_NAME_LENGTH + 1];
} message_hdr_t;

/**
 *  @struct message_data_hdr_t
 *  @brief header della parte dati
 */
typedef struct {
	/**
	 * Nickname del ricevente
	 */
	char receiver[MAX_NAME_LENGTH + 1];

	/**
	 * Lunghezza del buffer dati
	 */
	unsigned int len;
} message_data_hdr_t;

/**
 *  @struct message_data_t
 *  @brief body del messaggio 
 */
typedef struct {
	/**
	 * Header della parte dati
	 */
	message_data_hdr_t hdr;

	/**
	 * Buffer dati
	 */
	char *buf;
} message_data_t;

/**
 *  @struct message_t
 *  @brief tipo del messaggio 
 *
 */
typedef struct {
	/**
	 * Header del messaggio
	 */
	message_hdr_t hdr;
	/**
	 * Dati del messaggio
	 */
	message_data_t data;
} message_t;

/* ------ funzioni di utilità ------- */

/**
 * @function setheader
 * @brief scrive l'header del messaggio
 *
 * @param hdr puntatore all'header
 * @param op tipo di operazione da eseguire
 * @param sender mittente del messaggio
 */
static inline void setHeader(message_hdr_t *hdr, op_t op, char *sender) {
#if defined(MAKE_VALGRIND_HAPPY)
	memset((char*)hdr, 0, sizeof(message_hdr_t));
#endif
	hdr->op = op;
	strncpy(hdr->sender, sender, strlen(sender) + 1);
}
/**
 * @function setData
 * @brief scrive la parte dati del messaggio
 *
 * @param msg puntatore al body del messaggio
 * @param rcv nickname o groupname del destinatario
 * @param buf puntatore al buffer 
 * @param len lunghezza del buffer
 */
static inline void setData(message_data_t *data, char *rcv, const char *buf,
		unsigned int len) {
#if defined(MAKE_VALGRIND_HAPPY)
	memset((char*)&(data->hdr), 0, sizeof(message_data_hdr_t));
#endif

	strncpy(data->hdr.receiver, rcv, strlen(rcv) + 1);
	data->hdr.len = len;
	data->buf = (char *) buf;
}

#endif /* MESSAGE_H_ */
