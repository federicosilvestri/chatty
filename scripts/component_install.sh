#!/bin/bash

echo "Installing component for project"

# Checking the existence of sudo command...

su_cmd=""

if [ -x "$(command -v sudo)" ]; then
	su_cmd="sudo"
fi

$su_cmd apt install -y libconfig-dev libconfig9
