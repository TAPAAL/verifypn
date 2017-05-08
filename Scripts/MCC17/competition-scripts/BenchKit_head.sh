#!/bin/bash

# This is the initialization script for the participation of TAPAAL
# untimed sequntial engine verifypn in the Petri net competition 2017.

# BK_EXAMINATION: it is a string that identifies your "examination"
# BK_TOOL: it is the name of the TAPAAL tool variant to be invoked
# export PATH="$PATH:/home/mcc/BenchKit/bin/"

PREFIX=/home/mcc/BenchKit

case "$BK_TOOL" in
    *)
        echo "---> " $BK_TOOL " --- TAPAAL"
        exec $PREFIX/tapaal.sh
                ;;
    other)
        echo "---> Error: Unrecognized BK_TOOL name !!!"  
        exit 0
        ;;
esac
