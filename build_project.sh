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

echo "Trying to build chatty..."

echo "Cleaning up..."
rm -rf Release
mkdir -p Release/src

echo "Copying files..."
cp -r ./src/* Release/
cp -r ./include/* Release/
cp ./makefiles/Makefile Release/
cp -r ./scripts/* Release/
cp -r ./DATA Release/
cp -r doc Release/

chmod a+x ./Release/*.sh

echo "Launching make..."
cd Release
ls -a

if [ $x $1 ]; then
    echo "Making deps"
    make deps
fi

echo "Making chatty"
make chatty

echo "Making docs"
cd ..
doxygen
