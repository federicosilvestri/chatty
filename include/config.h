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

#define MAX_NAME_LENGTH	32
#define CONFIG_REQUIRED_PARAMS_SIZE 8
#define CONF_STRING_T 0
#define CONF_INT_T 1

static const char *config_req_params[] = { "UnixPath", "MaxConnection",
		"ThreadsInPool", "MaxMsgSize", "MaxFileSize", "MaxHistsMsg", "Dirname",
		"StatFileName" };

static const char config_req_params_type[] = { CONF_STRING_T, CONF_INT_T, CONF_INT_T, \
		CONF_INT_T, CONF_INT_T, CONF_INT_T, CONF_INT_T, CONF_INT_T };

// to avoid warnings like "ISO C forbids an empty translation unit"
typedef int make_iso_compilers_happy;

#endif /* CONFIG_H_ */
