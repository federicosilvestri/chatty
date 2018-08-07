/*
 * membox Progetto del corso di LSO 2017/2018
 *
 * Dipartimento di Informatica Università di Pisa
 * Docenti: Prencipe, Torquati
 * 
 */
/**
 * @file config.h
 * @brief File contenente alcune define con valori massimi utilizzabili
 */

#if !defined(CONFIG_H_)
#define CONFIG_H_

#include <stdbool.h>

/**
 * Max length of user name
 */
#define MAX_NAME_LENGTH	32

/**
 * Represents the string type of configuration parameter
 */
#define CONF_STRING_T 0

/**
 * Represents the integer type of configuration parameter
 */
#define CONF_INT_T 1

/**
 * @brief This function load into memory the file passed as configuration file.
 * If any problem occurs during loading it returns false.
 * @param conf_file_path string that represents the path of config file.
 *
 */
bool config_parse(char *conf_file_path);

/**
 * @brief clean workspace from any leaks.
 */
void config_clean();


#endif /* CONFIG_H_ */
