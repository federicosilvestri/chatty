/*
 * server.h
 *
 *  Created on: Aug 4, 2018
 *      Author: federicosilvestri
 */

#ifndef SERVER_H_
#define SERVER_H_

#include <stdbool.h>
#include <libconfig.h>

extern config_t server_conf;

bool producer_init();

#endif /* SERVER_H_ */
