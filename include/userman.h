/*******************************************************************************
 * SOL 2017/2018
 * Chatty
 * Federico Silvestri 559014
 * Si dichiara che il contenuto di questo file e' in ogni sua parte opera
 * originale dell'autore.
 *******************************************************************************/
#ifndef USERMAN_H

#include <stdbool.h>

#include "message.h"

/**
 * A very primitive data structure
 * for user.
 */
typedef struct {
	/**
	 * Username of registered user.
	 */
	char *username;
	/**
	 * A timestamp format of last login.
	 */
	long last_login;
} userman_user_t;


/**
 * Constant for user list function.
 * This option will tell function to return all users.
 */
#define USERMAN_GET_ALL 0

/**
 * Constant for user list function.
 * This option will tell function to return only online users.
 */
#define USERMAN_GET_ONL 1

/**
 * Constant for user list function.
 * This option will tell function to return only offline users.
 */
#define USERMAN_GET_OFFL 2

/**
 * Constant for user set status function.
 * This option will set user in online status
 */
#define USERMAN_STATUS_ONL true

/**
 * Constant for user set status function.
 * This option will set user in offline status
 */
#define USERMAN_STATUS_OFFL false

/**
 * Constant to get only read messages
 */
#define USERMAN_GET_MSGS_READ 1

/**
 *
 * Constant to get only unread messages
 */
#define USERMAN_GET_MSGS_UNREAD 0

/**
 * Constant to get all messages (read and unread)
 * @return
 */
#define USERMAN_GET_MSGS_ALL 2

/**
 * Initializes userman.
 * @return true on success, false on error.
 */
bool userman_init();

/**
 * This function checks if user is already registered or not.
 * @brief check if user exists
 * @param nickname of user
 * @return true if exists, false if not
 */
bool userman_user_exists(char* nickname);

/**
 * This function adds user to system, in case of error a specific
 * return value will be returned.
 *
 * @brief add a new user to system
 * @param nickname of a new user
 * @return 0 on success, 1 on already exists, 2 on invalid nickname, 3 on other fatal error
 */
int userman_add_user(char *nickname);

/**
 * This function will delete a user from system, in case of
 * error a specific return value will be returned.
 *
 * @param nickname that identifies user to delete
 * @return true on success, false on error
 */
bool userman_delete_user(char* nickname);

/**
 * This functions returns an array
 * that contains all registered usernames
 * Note that array is composed
 *
 * @param option an option available ALL, ONL, OFFL
 * @param list string a pointer to string (internally allocated and built)
 * @return -1 if any error occurs, else the number of the selected user.
 */
int userman_get_users(char option, char** list);

/**
 * Update the status of user: online or offline.
 *
 * @param nickname the nickname which is online
 * @param status set true if online, false if offline
 * @return false in case of error, true in case of success
 */
bool userman_set_user_status(char* nickname, bool status);

/**
 * Checks if user is online or not.
 *
 * @param nickname user to check the status
 * @return true if online, false if not
 */
bool userman_user_is_online(char *nickname);

/**
 * Add message to database.
 *
 * @param sender sender of message
 * @param receiver recipient of message
 * @param read true if message is read, false if it's not read
 * @param body the body of message
 * @param is_file true if file is sent, false if it's not sent.
 * @return true on success, false on failure.
 */
bool userman_add_message(char *sender, char *receiver, bool read, char *body,
bool is_file);

/**
 * Add a message for all users to database.
 *
 * @param nickname the nickname of sender
 * @param is_file true if is file, false if not
 * @param body the body of messae.
 * @return true on success, false on error
 */
bool userman_add_broadcast_msg(char *nickname, bool is_file, char *body);

/**
 * Get the messages of users.
 *
 * @param nickname that identifies the user
 * @param messages the pointer to the message list
 * @param senders an array that contains sender of message, set NULL  if don't want
 * @param ids the ids of messages, set to NULL if you don't want
 * @param files the pointer to the bool is_file list
 * @param read set 0 to get messages that are not read, 1 to get messages that are read,
 * @param limit how many message you want to retrieve
 * @return -1 in case of error, the number of messages in case of success
 */
int userman_get_msgs(char* nickname, char*** messages, char ***senders, int **ids, bool** files, unsigned short int read, int limit);

/**
 * Free the memory used to store messages
 *
 * @param messages message list pointer
 * @param senders  sender list pointer
 * @param msg_size how many messages we have in the array
 * @param ids ids array pointer
 * @param is_files is_file array pointer
 */
void userman_free_msgs(char ***messages, char ***senders, int msg_size, int **ids, bool **is_files);

/**
 * Set message status, read or unread.
 * @param msgid the id of message
 * @param read true if you want to set to read, false to set to unread
 * @return true on success, false on error
 */
bool userman_set_msg_status(int msgid, bool read);

/**
 * Store a file inside FileDir
 *
 * @param filename the name of the file
 * @param buffer buffer of file
 * @param bufflen the length of the buffer
 * @return true on success, false on erro
 */
bool userman_store_file(char *filename, char *buffer, unsigned int bufflen);

/**
 * Checks if there are available file called filename for nickname user.
 * @param filename the file name to check
 * @param nickname the nickname to check for
 * @return true if it exists, false if doesn't exist.
 */
bool userman_file_exists(char *filename, char *nickname);

/**
 * This function searches if there are files
 * with name filename in the datastore.
 *
 * @param filename the filename to search
 * @return true if exists, false if not exists
 */
bool userman_search_file(char *filename);

/**
 * Get a file from datastore.
 *
 * @param filename the file name you want to get.
 * @param buf the buffer where you want function pushes data.
 * @return -1 in case of error, in case of success the size of file
 */
size_t userman_get_file(char *filename, char **buf);

/**
 * Deallocate and destroy userman.
 */
void userman_destroy();

#define USERMAN_H
#endif /* USERMAN_H */
