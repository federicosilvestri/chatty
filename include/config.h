/*******************************************************************************
 * SOL 2017/2018
 * Chatty
 * Federico Silvestri 559014
 * Si dichiara che il contenuto di questo file e' in ogni sua parte opera
 * originale dell'autore.
 *******************************************************************************/

/**
 * @file config.h
 * @brief File contenente alcune define con valori massimi utilizzabili
 */

#if !defined(CONFIG_H_)
#define CONFIG_H_

#include <stdbool.h>

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
