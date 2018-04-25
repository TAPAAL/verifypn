#!/bin/bash

S1="$1"
B1="$2"
S2="$3"
B2="$4"

for t in {Reachability,CTL}{Cardinality,Fireability} StateSpace UpperBounds ReachabilityDeadlock ; do 
    pushd BENCHKIT/$S1/$B1 > /dev/null
    grep -P '(STATE_SPACE|FORMULA)' *.$t | grep -oP '.*(?=TECHNIQUES)' | sort | uniq > ../../../answers/$S1.$B1.$t
    popd > /dev/null
    pushd BENCHKIT/$S2/$B2 > /dev/null
    grep -P '(STATE_SPACE|FORMULA)' *.$t | grep -oP '.*(?=TECHNIQUES)' | sort | uniq > ../../../answers/$S2.$B2.$t
    popd > /dev/null
    LEFT=$(comm -13 answers/$S1.$B1.$t answers/$S2.$B2.$t )
    RIGHT=$(comm -23 answers/$S1.$B1.$t answers/$S2.$B2.$t )
    echo "DIFFERENCES IN $t : "
    comm -12 <(echo "$LEFT" | grep -oP '(?<= ).*-[0-9]+(?= )' | sort) <(echo "$RIGHT"  | grep -oP '(?<= ).*-[0-9]+(?= )' | sort)
    C1=$(cat answers/$S1.$B1.$t | wc -l)
    C2=$(cat answers/$S2.$B2.$t | wc -l)
    echo "$S1.$B1.$t -> $C1"
    echo "$S2.$B2.$t -> $C2"
done 
