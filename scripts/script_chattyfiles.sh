#!/bin/bash
#*******************************************************************************
# SOL 2017/2018
# Chatty
# Federico Silvestri 559014
# Si dichiara che il contenuto di questo file e' in ogni sua parte opera
# originale dell'autore.
#*******************************************************************************

# usage function
function usage() {
	echo "To use this script you must pass chatty configuration file and t>0 minutes" >&2
	echo -e "\tusage: $0 <config-file> <minutes>" >&2
}

# checking the -help on the arguments
for cmd_help in "$@"; do
	if [ $cmd_help == "-help" ]; then
		usage
		exit 1
	fi
done

# checking the argument numberes
if [ $# -ne 2 ]; then
	usage
	exit 1
fi

# checking the arguments format
n_regex='^[+-]?[0-9]+([.][0-9]+)?$'
if ! [[ $2 =~ $n_regex ]] ; then
   echo "\"$2\" is not a number!" >&2
   exit 1
fi

# checking if paramater time is less than 1
if [ $2 -lt 0 ]; then
	echo "Time parameter must be >= 0!"
	exit 1
fi

# checking if configuration file exists
if ! [ -r $1 ] || ! [ -s $1 ] || [ -d $1 ]; then
	echo "The file $1 is not readable, it's empty, or is a directory!"
	exit 1
fi

chatty_dirname=""

# parsing the configuration file
IFS_S=$IFS
IFS="="

while read -r conf_name conf_value; do
	if [[ $conf_name = *"DirName"* ]]; then
		chatty_dirname=$conf_value
	fi
done < $1

IFS=$IFS_S

# removing all white spaces
chatty_dirname="$(echo -e "${chatty_dirname}" | tr -d '[:space:]')"
# removing ""
chatty_dirname="$(echo -e "${chatty_dirname}" | tr -d '\"')"

echo "Inspecting $chatty_dirname..."

# Checking if argument is a directory or not
if ! [ -d $chatty_dirname ]; then
	echo "\"$chatty_dirname\" is not a directory!"
	exit 1 
fi

if [ $2 = 0 ]; then
	# listing all files
	for chatty_file in $(find $chatty_dirname -mmin +$2 -type f); do
		echo -e "$chatty_file"
	done
else
	# creating archive name
	archive_name="archive-$(date +%s).tar.gz"
	
	# creating a list for archive
	to_archive=($(find $chatty_dirname -mmin +$2 -type f))
	
	if [ ${#to_archive[@]} = 0 ]; then
		echo "No files available!"
		exit 0
	fi
	
	# creating a manifest file
	echo "Created by $(who) on $(date)" > manifest.txt
	
	# creating an empty archive
	tar -cvf $archive_name manifest.txt "${to_archive[@]}" 
	
	# Removing temporary manifest
	rm manifest.txt
	
	# Removing files
	rm "${to_archive[@]}"
	
	echo "Created archive $archive_name with older files" 
fi


exit 0
