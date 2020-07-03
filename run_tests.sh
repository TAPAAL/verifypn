#!/bin/bash

B=$1
O=$2
F=$3
T="$4"

if [ -z "$B" ] ; then
	echo "Missing binary"
	exit
fi
if [ -z "$O" ] ; then
	echo "Missing options"
	exit
fi
if [ -z "$F" ] ; then
	echo "Missing output"
	exit
fi

if [ -z "$T" ] ; then
	echo "No timing given, using 5 seconds"
	T=5
fi




echo "" > $F
for f in $(ls test_models) ; do
	echo "Run $f" 
	NP=$(grep "<property>" "test_models/$f/query.xml" | wc -l)
	for Q in $(seq 1 $NP ) ; do 
		echo "	Q$Q"
		res=$(eval "timeout $T $1 $2 -x $Q test_models/$f/model.pnml test_models/$f/query.xml " | grep "Query is ")
		if [ ! -z "$res" ] ; then
			echo "$f:$Q:$res" >> $F
		fi
	done
done
