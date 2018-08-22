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
bool userman_user_exists(char*);

/**
 * This function adds user to system, in case of error a specific
 * return value will be returned.
 *
 * @brief add a new user to system
 * @param nickname of a new user
 * @return 0 on success, 1 on already exists, 2 on invalid nickname, 3 on other fatal error
 */
int userman_add_user(char*);

/**
 * This function will delete a user from system, in case of
 * error a specific return value will be returned.
 *
 * @param nickname that identifies user to delete
 * @return true on success, false on error
 */
bool userman_delete_user(char*);

/**
 * This functions returns an array
 * that contains all registered usernames
 * Note that array is composed
 *
 * @param option an option available ALL, ONL, OFFL
 * @param string a pointer to string (internally allocated and built)
 * @return -1 if any error occurs, else the number of the selected user.
 */
size_t userman_get_users(char, char**);

/**
 * Update the status of user: online or offline.
 *
 * @param nickname the nickname which is online
 * @param status set true if online, false if offline
 * @return false in case of error, true in case of success
 */
bool userman_set_user_status(char*, bool);

/**
 * Deallocate and destroy userman.
 */
void userman_destroy();

#define USERMAN_H
#endif /* USERMAN_H */
