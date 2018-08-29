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
 * Read the payload of a message.
 *
 * @param connfd connection file descriptor
 * @param buffer the buffer where write the payload
 * @param length the length of payload to read
 * @return -1 in case of error 1 in case of success
 */
int readPayload(long connfd, char *buffer, unsigned int length) {
	while (length > 0) {
		ssize_t read_return = read(connfd, buffer, length);

		if (read_return <= 0) {
			return (int) read_return;
		}

		// address summing!
		buffer += read_return;
		length -= (int) read_return;
	}

	return 1;
}

/**
 * Write a payload of a message.
 *
 * @param connfd connection file descriptor
 * @param buffer the buffer to write in the socket
 * @param length the length of the buffer
 * @return -1 in case of error, 1 in case of success
 */
int writePayload(long connfd, char *buffer, unsigned int length) {
	while (length > 0) {
		int w_bytes = write(connfd, buffer, length);

		if (w_bytes <= 0) {
			return w_bytes;
		}

		// address summing!
		buffer += w_bytes;
		length -= w_bytes;
	}

	return 1;
}

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

	int header_len = sizeof(message_hdr_t);
	message_hdr_t *hdr_cursor = hdr;
	memset(hdr, 0, header_len);

	while (header_len > 0) {
		int read_bytes = read(connfd, hdr_cursor, header_len);

		if (read_bytes <= 0) {
			return read_bytes;
		}

		hdr_cursor += read_bytes;
		header_len -= read_bytes;
	}

	return 1;
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

	int data_hdr_size = sizeof(message_data_hdr_t);
	message_data_hdr_t *data_hdr_cursor = &data->hdr;

	while (data_hdr_size > 0) {
		int read_size = read(connfd, data_hdr_cursor, data_hdr_size);

		if (read_size <= 0) {
			return read_size;
		}

		data_hdr_cursor += read_size;
		data_hdr_size -= read_size;

	}

	// success

	// read payload, if necessary
	if (data->hdr.len > 0) {
		// prepare buffer
		data->buf = malloc(data->hdr.len * sizeof(char));
		memset(data->buf, 0, data->hdr.len * sizeof(char));

		int payload_s = readPayload(connfd, data->buf, data->hdr.len);

		if (payload_s <= 0) {
			free(data->buf);
			return payload_s;
		}

	} else {
		data->buf = NULL;
	}

	return 1;
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

	// read header
	int h_s = readHeader(connfd, &msg->hdr);
	if (h_s <= 0) {
		return h_s;
	}

	// read data
	int p_s = readData(connfd, &msg->data);

	if (p_s <= 0) {
		return p_s;
	}

	return 1;
}

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

	// writing header of message
	int h_s = sendHeader((int) fd, &msg->hdr);

	if (h_s <= 0) {
		return h_s;
	}

	// writing data
	int p_s = sendData(fd, &msg->data);

	if (p_s <= 0) {
		return p_s;
	}

	return 1;
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
	message_data_hdr_t *message_hdr_cursor = &msg->hdr;
	int header_w_size = sizeof(message_data_hdr_t);

	while (header_w_size > 0) {
		int d_status = write(fd, message_hdr_cursor, header_w_size);

		if (d_status <= 0) {
			return d_status;
		}

		message_hdr_cursor += d_status;
		header_w_size -= d_status;
	}

	// send payload
	int p_status = writePayload(fd, msg->buf, msg->hdr.len);
	if (p_status <= 0) {
		return p_status;
	}

	return 1;
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

	int header_size = sizeof(message_hdr_t);

	while (header_size > 0) {
		int w_size = write(fd, hdr, header_size);

		if (w_size <= 0) {
			return (int) w_size;
		}

		hdr += w_size;
		header_size -= w_size;
	}

	return 1;
}
