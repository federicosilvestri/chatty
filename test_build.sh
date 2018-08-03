#!/bin/bash

echo "Cleaning up..."
rm -rf TestDebug
mkdir -p TestDebug/src

echo "Copying files..."
cp -r ./src/* TestDebug/
cp -r ./include/* TestDebug/
cp ./makefiles/Makefile TestDebug/
cp -r scripts/* TestDebug/

chmod a+x TestDebug/*.sh

echo "Launching make..."
cd TestDebug
make dev-test
