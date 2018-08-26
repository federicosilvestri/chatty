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
size_t userman_get_users(char option, char** list);

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
 * Get the previous messages of users.
 *
 * @param nickname that identifies the user
 * @param messages the pointer to the message list
 * @param files the pointer to the bool is_file list
 * @return -1 in case of error, the number of messages in case of success
 */
int userman_get_prev_msgs(char* nickname, char*** messages, bool** files);

/**
 * Deallocate and destroy userman.
 */
void userman_destroy();

#define USERMAN_H
#endif /* USERMAN_H */
