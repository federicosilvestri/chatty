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

	ssize_t to_read = sizeof(message_hdr_t);
	ssize_t read_size = 0;

	while (to_read > 0) {
		ssize_t read_bytes = read((int) connfd, hdr, (size_t) to_read);

		if (read_bytes <= 0) {
			return (int) read_bytes;
		}

		to_read -= read_bytes;
		read_size += read_bytes;
	}

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

	long int data_hdr_size = 0;
	long int data_payload_size = 0;

	memset(data, 0, sizeof(message_data_t));

	data_hdr_size = read((int) connfd, &data->hdr, sizeof(message_data_hdr_t));

	if (data_hdr_size <= 0) {
		return (int) data_hdr_size;
	}

	// read payload, if necessary
	if (data->hdr.len > 0) {
		data->buf = calloc(sizeof(char), data->hdr.len);
		unsigned int to_read = data->hdr.len;

		while (to_read > 0) {
			long int t_size = read((int) connfd, data->buf, data->hdr.len);

			if (t_size < 0) {
				// error
				return (int) t_size;
			}

			data_payload_size += t_size;
			to_read -= (unsigned int) t_size;
		}

	}

	return (int) (data_hdr_size + data_payload_size);
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

	int h_size = 0;
	int p_size = 0;

	// initialize message
	memset(msg, 0, sizeof(message_t));

	// read header
	h_size = readHeader(connfd, &msg->hdr);
	if (h_size <= 0) {
		return h_size;
	}

	// read data
	p_size = readData(connfd, &msg->data);

	if (p_size <= 0) {
		return p_size;
	}

	return (h_size + p_size);
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

	int h_size = 0;
	int p_size = 0;

	// writing header of message
	h_size = sendHeader((int) fd, &msg->hdr);

	if (h_size <= 0) {
		return h_size;
	}

	// writing data
	p_size = sendData(fd, &msg->data);

	if (p_size <= 0) {
		return p_size;
	}

	return (h_size + p_size);
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

	// read header, first
	ssize_t dh_size = 0;
	dh_size = write((int) fd, &msg->hdr, sizeof(message_data_hdr_t));

	if (dh_size <= 0 || msg->hdr.len == 0) {
		return (int) dh_size;
	}

	ssize_t payload_wrt_size = 0;

	// write payload
	ssize_t remain_size = msg->hdr.len;
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

	ssize_t to_write = sizeof(message_hdr_t);
	ssize_t w_size = 0;

	while (to_write > 0) {
		ssize_t written_size = write(fd, hdr, (size_t) to_write);

		if (written_size < 0) {
			return (int) written_size;
		}

		w_size += written_size;
		to_write -= written_size;
	}

	return (int) w_size;
}

