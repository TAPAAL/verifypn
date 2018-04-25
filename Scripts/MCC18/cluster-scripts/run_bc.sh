#!/bin/bash

BIN="$1"
F=MCC17-COL
SCRIPTS=COMP51-COL

mkdir -p "output/${BIN}-TIMED/"
for t in ReachabilityCardinality ReachabilityFireability CTLCardinality CTLFireability ReachabilityDeadlock; do #UpperBounds StateSpace; do # StateSpace; do # ReachabilityDeadlock CTLCardinality CTLFireability UpperBounds; do # ReachabilityDeadlock; do #{Reachability,CTL}{Fireability,Cardinality} ; do #,Reachability}{Fireability,Cardinality}  ReachabilityDeadlock UpperBounds ; do # StateSpace ReachabilityDeadlock ; do # UpperBounds ; do
    for i in $(ls $F) ; do 
        sbatch -n 1 -c 4  ./run_benchkit.sh $SCRIPTS $BIN $i $t $F
    done
done 
