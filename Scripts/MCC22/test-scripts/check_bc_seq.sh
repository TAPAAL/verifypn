#!/bin/bash

STRATEGY1="$1"
STRATEGY2="$2"
MODELS="$3"
BINARY="$4"
DIR1="output/$STRATEGY1/$MODELS/$BINARY"
DIR2="output/$STRATEGY2/$MODELS/$BINARY"
OUTPUT="time/$MODELS.$BINARY.$STRATEGY1.$STRATEGY2"
ANSWERS="../answers/unique/$MODELS.$BINARY"

for t in Reachability{Cardinality,Fireability} ; do
    pushd output/$STRATEGY1/$MODELS/$BINARY > /dev/null
    # awk is here to take only the answers of the sequential processing
    awk '/Sequential processing/,0' *.$t | grep -aP 'FORMULA' | grep -oP '.*(?=TECHNIQUES)' | sed "s/-$t//g" | grep -v '?' | sort | uniq > $ANSWERS.$STRATEGY1.$t
    popd > /dev/null
    pushd output/$STRATEGY2/$MODELS/$BINARY > /dev/null
    awk '/Sequential processing/,0' *.$t | grep -aP 'FORMULA' | grep -oP '.*(?=TECHNIQUES)' | sed "s/-$t//g" | grep -v '?' | sort | uniq > $ANSWERS.$STRATEGY2.$t
    popd > /dev/null

    LEFT=$(comm -13 $ANSWERS.$STRATEGY1.$t $ANSWERS.$STRATEGY2.$t )
    RIGHT=$(comm -13 $ANSWERS.$STRATEGY2.$t $ANSWERS.$STRATEGY1.$t )

    echo "--- $t ---"
    consistency=$(comm -12 <(echo "$LEFT" | grep -oP '^.*:FORMULA [^ ]+' | sort) <(echo "$RIGHT"  | grep -oP '^.*:FORMULA [^ ]+' | sort))
    if [[ ! -n "$consistency" ]] ; then
        echo "The results are consistent."
    else
        echo "The following results are inconsistent:"
        echo "$consistency"
    fi

    unique_answers1=$(comm -13 <(echo "$LEFT" | grep -oP '^.*:FORMULA [^ ]+' | sort) <(echo "$RIGHT"  | grep -oP '^.*:FORMULA [^ ]+' | sort))
    unique_answers2=$(comm -23 <(echo "$LEFT" | grep -oP '^.*:FORMULA [^ ]+' | sort) <(echo "$RIGHT"  | grep -oP '^.*:FORMULA [^ ]+' | sort))
    echo "Number of unique answers given by $B1: $(echo "$unique_answers1" | wc -l)"
    echo "Number of unique answers given by $B2: $(echo "$unique_answers2" | wc -l)"

    C1=$(cat answers/$S1.$B1.$t | wc -l)
    C2=$(cat answers/$S2.$B2.$t | wc -l)
    echo "$S1.$B1.$t -> $C1 queries answered"
    echo "$S2.$B2.$t -> $C2 queries answered"
done