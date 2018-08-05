#!/bin/bash


LD_PRELOAD=/usr/lib/gcc/x86_64-linux-gnu/7/libasan.so DebugClang/chatty -f $1
