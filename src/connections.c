/*******************************************************************************
 * SOL 2017/2018
 * Chatty
 * Federico Silvestri 559014
 * Si dichiara che il contenuto di questo file e' in ogni sua parte opera
 * originale dell'autore.
 *******************************************************************************/
#include "connections.h"

#include <stdio.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <errno.h>

/**
 * @brief Apre una connessione AF_UNIX verso il server
 *
 * @param path Path del socket AF_UNIX
 * @param ntimes numero massimo di tentativi di retry
 * @param secs tempo di attesa tra due retry consecutive
 *
 * @return il descrittore associato alla connessione in caso di successo
 *         -1 in caso di errore
 */
int openConnection(char* path, unsigned int ntimes, unsigned int secs) {
	// create socket
	int fd;
	struct sockaddr_un socket_address;
	unsigned int retries, sleeping;
	int conn_status;

	fd = socket(AF_UNIX, SOCK_STREAM, 0);

	if (fd < 0) {
		return -1;
	}

	socket_address.sun_family = AF_UNIX;
	strncpy(socket_address.sun_path, path, UNIX_PATH_MAX);

	// check parameters validity
	retries = (ntimes < MAX_RETRIES) ? ntimes : MAX_RETRIES;
	sleeping = (secs < MAX_SLEEPING) ? secs : MAX_SLEEPING;

	do {
		retries -= 1;
		conn_status = connect(fd, (const struct sockaddr*) &socket_address,
				sizeof(socket_address));

		if (conn_status != 0) {
			if (errno == ENOENT) {
				sleep(sleeping);
			} else {
				return -1;
			}
		}

	} while (retries > 0 && conn_status != 0);

	return fd;
}

// -------- server side -----
/**
 * @brief Legge l'header del messaggio
 *
 * @param connfd     descrittore della connessione
 * @param hdr    puntatore all'header del messaggio da ricevere
 *
 * @return <=0 se c'e' stato un errore
 *         (se <0 errno deve essere settato, se == 0 connessione chiusa)
 */
int readHeader(long connfd, message_hdr_t *hdr) {
	if (connfd < 0) {
		errno = ENOENT;
		return -1;
	}

	memset(hdr, 0, sizeof(message_hdr_t));
	int read_size = read(connfd, hdr, sizeof(message_hdr_t));

	return read_size;
}

/**
 * @brief Legge il body del messaggio
 *
 * @param fd     descrittore della connessione
 * @param data   puntatore al body del messaggio
 *
 * @return <=0 se c'e' stato un errore
 *         (se <0 errno deve essere settato, se == 0 connessione chiusa)
 */
int readData(long connfd, message_data_t *data) {
	if (connfd < 0) {
		errno = ENOENT;
		return -1;
	}

	memset(data, 0, sizeof(message_data_t));
	int read_size = read(connfd, data, sizeof(message_data_t));

	return read_size;
}

/**
 * @brief Legge l'intero messaggio
 *
 * @param fd     descrittore della connessione
 * @param msg   puntatore al messaggio
 *
 * @return <=0 se c'e' stato un errore
 *         (se <0 errno deve essere settato, se == 0 connessione chiusa)
 */
int readMsg(long connfd, message_t *msg) {
	if (connfd < 0) {
		errno = ENOENT;
		return -1;
	}

	// initialize message
	memset(msg, 0, sizeof(message_t));

	// reading
	int read_size = read(connfd, msg, sizeof(message_t));

	// checking
	if (read_size <= 0) {
		return read_size;
	}

	return 1;
}

/* da completare da parte dello studente con altri metodi di interfaccia */

// ------- client side ------
/**
 * @brief Invia un messaggio di richiesta al server
 *
 * @param fd     descrittore della connessione
 * @param msg    puntatore al messaggio da inviare
 *
 * @return <=0 se c'e' stato un errore
 */
int sendRequest(long fd, message_t *msg) {
	if (fd < 0) {
		return -1;
	}

	int w_size = write(fd, msg, sizeof(message_t));

	if (w_size <= 0) {
		return -1;
	}

	return 0;
}

/**
 * @brief Invia il body del messaggio al server
 *
 * @param fd     descrittore della connessione
 * @param msg    puntatore al messaggio da inviare
 *
 * @return <=0 se c'e' stato un errore
 */
int sendData(long fd, message_data_t *msg) {
	if (fd < 0) {
		return -1;
	}

	int w_size = write(fd, msg, sizeof(msg));

	if (w_size <= 0) {
		return -1;
	}

	return 0;
}
