/*******************************************************************************
 * SOL 2017/2018
 * Chatty
 * Federico Silvestri 559014
 * Si dichiara che il contenuto di questo file e' in ogni sua parte opera
 * originale dell'autore.
 *******************************************************************************/
#include "connections.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <errno.h>
#include <unistd.h>

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
	unsigned int retries, sleeping;
	// check parameters validity
	retries = (ntimes < MAX_RETRIES) ? ntimes : MAX_RETRIES;
	sleeping = (secs < MAX_SLEEPING) ? secs : MAX_SLEEPING;

	// first check if file exists
	bool stop = false;
	for (unsigned int i = 0; i < ntimes && stop == false; i++) {
		if (access(path, F_OK) == -1) {
			// file does not exist
			sleep(sleeping);
		} else {
			stop = true;
		}
	}

	if (stop == false) {
		errno = ENOENT;
		return -1;
	}

	// creating socket
	int fd;
	struct sockaddr_un socket_address;
	int conn_status;
	fd = socket(AF_UNIX, SOCK_STREAM, 0);

	if (fd < 0) {
		return -1;
	}

	socket_address.sun_family = AF_UNIX;
	strncpy(socket_address.sun_path, path, UNIX_PATH_MAX);

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
	ssize_t read_size = read((int) connfd, hdr, sizeof(message_hdr_t));

	return (int) read_size;
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
	data->hdr.len = 3;
	long int dh_size = read((int) connfd, &data->hdr, sizeof(message_data_hdr_t));

	if (dh_size <= 0) {
		return (int) dh_size;
	}

	// read payload
	data->buf = calloc(sizeof(char), data->hdr.len);

	size_t to_read = (size_t) data->hdr.len;
	size_t payload_size = 0;

	while (to_read > 0) {
		long int t_size = read((int) connfd, data->buf, data->hdr.len);
		payload_size += (size_t) t_size;
		to_read -= (size_t) t_size;
	}


	if (payload_size <= 0) {
		return (int) payload_size;
	}

	return (int) (dh_size + (int) payload_size);
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
	long int read_size = read((int) connfd, msg, sizeof(message_t));

	// checking
	if (read_size <= 0) {
		return (int) read_size;
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

	ssize_t w_size = write((int) fd, msg, sizeof(message_t));

	if (w_size <= 0) {
		return -1;
	}

	return 0;
}

/**
 * @brief Invia il body del messaggio al server, oppure il server al client.
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

	ssize_t dh_size = write((int) fd, &msg->hdr, sizeof(message_data_hdr_t));

	if (dh_size <= 0) {
		return (int) dh_size;
	}

	// write payload
	ssize_t remain_size = msg->hdr.len;
	ssize_t payload_wrt_size = 0;

	while (remain_size > 0) {
		ssize_t written_size = write((int) fd, msg->buf, (size_t) remain_size);

		if (written_size < 0) {
			return (int) written_size;
		}

		remain_size -= written_size;
		payload_wrt_size += written_size;
	}

	if (payload_wrt_size <= 0) {
		return (int) payload_wrt_size;
	}

	return (int) (dh_size + payload_wrt_size);
}

/**
 * @brief Invia l'header al client, utilizzato dal server per risposte veloci, bodyless.
 * @param fd file descriptor sul quale comunicare
 * @param hdr header da inviare
 * @return -1 if error, 0 if disconnected, sizeof write if success
 */
int sendHeader(int fd, message_hdr_t *hdr) {
	if (fd < 0) {
		return -1;
	}

	ssize_t w_size = write(fd, hdr, sizeof(hdr));

	return (int) w_size;
}

