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


LD_PRELOAD=/usr/lib/gcc/x86_64-linux-gnu/7/libasan.so DebugClang/chatty -f $1
