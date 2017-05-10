#!/bin/bash
ulimit -v $1; 
shift 
cmd="$@"
#echo $cmd
eval $cmd
