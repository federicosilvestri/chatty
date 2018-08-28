#!/bin/bash

echo "Testing Project" 

#for ((i = 0; i < 10; i++)); do
#    echo "Making test1 $i"
#    make test1
#done

echo "Making test1 $i"
make test1

pkill -QUIT chatty

make test2

pkill -QUIT chatty

echo "TEST PROJECT PASSED"
