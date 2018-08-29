#!/bin/bash
#*******************************************************************************
# SOL 2017/2018
# Chatty
# Federico Silvestri 559014
# Si dichiara che il contenuto di questo file e' in ogni sua parte opera
# originale dell'autore.
#*******************************************************************************

echo "Removing all components installed for Chatty"

su_cmd=""

if [ -x "$(command -v sudo)" ]; then
	su_cmd="sudo"
fi

echo "Stopping RabbitMQ Server..."

$su_cmd service rabbitmq-server stop

if [ $? -eq 0 ]; then
	echo "OK"
else
	echo "Problem during restarting"
	exit 1
fi

$su_cmd apt remove -qy --purge libconfig-dev libconfig9 rabbitmq-server librabbitmq4 librabbitmq-dev sqlite3 libsqlite3-dev net-tools psmisc

if ! [ $? -eq 0 ]; then
	echo "Problem during uninstall dependencies!"
	exit 1
fi

echo "All dependencies are removed, done!"