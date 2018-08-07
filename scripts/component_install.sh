#!/bin/bash

su_cmd=""
rabbitmq_port=5672

echo "Installing component for project"

if [ -x "$(command -v sudo)" ]; then
	su_cmd="sudo"
fi

$su_cmd apt install -y libconfig-dev libconfig9 rabbitmq-server librabbitmq4 librabbitmq-dev

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

echo "Checking if RabbitMQ is running..."
if [ -n "$($su_cmd service rabbitmq-server status | grep ' active ')" ]; then
	echo "OK"
else
	echo "Service is not running, throwing error..."
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
