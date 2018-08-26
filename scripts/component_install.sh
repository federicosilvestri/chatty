#!/bin/bash
#*******************************************************************************
# SOL 2017/2018
# Chatty
# Federico Silvestri 559014
# Si dichiara che il contenuto di questo file e' in ogni sua parte opera
# originale dell'autore.
#*******************************************************************************
#*******************************************************************************
# /*
#  * SOL 2017/2018
#  * Chatty
#  *
#  * \author Federico Silvestri 559014
#  *
#  * Si dichiara che il contenuto di questo file e' in ogni sua parte opera
#  * originale dell'autore.
#  *
# */
#*******************************************************************************

su_cmd=""
rabbitmq_port=5672

echo "Installing component for project"

if [ -x "$(command -v sudo)" ]; then
	su_cmd="sudo"
fi

$su_cmd apt install -y libconfig-dev libconfig9 rabbitmq-server librabbitmq4 librabbitmq-dev sqlite3 libsqlite3-dev

if ! [ $? -eq 0 ]; then
	echo "Problem during install dependencies!"
	exit 1
fi

echo "Restarting RabbitMQ server..."

$su_cmd service rabbitmq-server restart

if [ $? -eq 0 ]; then
	echo "OK"
else
	echo "Problem during restarting"
	exit 1
fi

echo "Checking if RabbitMQ is listening on port $rabbitmq_port"

if [ -n "$($su_cmd netstat -tlpn | grep ":$rabbitmq_port ")" ]; then
	echo "OK"
else
	echo "Error: RabbitMQ is not listening on correct port"
	exit 1
fi

echo "Component installation finished"
exit 0
