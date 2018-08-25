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
rm -rf TestDebug
mkdir -p TestDebug/src

echo "Copying files..."
cp -r ./src/* TestDebug/
cp -r ./include/* TestDebug/
cp ./makefiles/Makefile TestDebug/
cp -r ./scripts/* TestDebug/
cp -r ./DATA TestDebug/
chmod a+x ./TestDebug/*.sh

echo "Launching make..."
cd TestDebug
ls -a

echo "Making deps"
make deps

echo "Making chatty"
make chatty

for (( i=2; i <= 10; ++i ))
do
    echo "Make test1"
    make test1
done


echo "Making docs"
cd ..
doxygen
