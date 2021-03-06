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

if [[ $# != 1 ]]; then
    echo "usa $0 unix_path"
    exit 1
fi

rm -f valgrind_out
/usr/bin/valgrind --leak-check=full $1 -f DATA/chatty.conf1 >& ./valgrind_out &
pid=$!

# aspetto un po' per far partire valgrind
sleep 5

r=$(tail -10 ./valgrind_out | grep "ERROR SUMMARY" | cut -d: -f 2 | cut -d" " -f 2)

if [[ $r != 0 ]]; then
    echo "Test FALLITO"
    exit 1
fi

echo "Test OK!"
exit 0


