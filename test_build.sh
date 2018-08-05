#!/bin/bash

echo "Trying to build chatty..."

echo "Cleaning up..."
rm -rf TestDebug
mkdir -p TestDebug/src

echo "Copying files..."
cp -r ./src/* TestDebug/
cp -r ./include/* TestDebug/
cp ./makefiles/Makefile TestDebug/
cp -r ./scripts/* TestDebug/

chmod a+x ./TestDebug/*.sh

echo "Launching make..."
cd TestDebug
ls -a
make $1
make chatty