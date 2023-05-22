#!/bin/bash

STRATEGY1="$1"
STRATEGY2="$2"
MODELS="$3"
BINARY="$4"
DIR1="output/$STRATEGY1/$MODELS/$BINARY"
DIR2="output/$STRATEGY2/$MODELS/$BINARY"
OUTPUT="time/$MODELS.$BINARY.$STRATEGY1.$STRATEGY2"
ANSWERS="answers/$MODELS.$BINARY"
DIR=$(pwd)

for t in Reachability{Cardinality,Fireability} ; do
    pushd $DIR1 > /dev/null
    rm -f $DIR/$ANSWERS.$STRATEGY1.sequential.$t
    rm -f $DIR/$ANSWERS.$STRATEGY1.simplification.$t
    for f in $(ls *.$t) ; do
        awk '/Sequential processing/,0' $f | grep -aP 'FORMULA' | grep -oP '.*(?=TECHNIQUES)' | sed "s/-$t//g" | grep -v '?' | sort | uniq >> $DIR/$ANSWERS.$STRATEGY1.sequential.$t
        awk '1;/Sequential processing/{exit}' $f | grep -aP 'FORMULA' | grep -oP '.*(?=TECHNIQUES)' | sed "s/-$t//g" | grep -v '?' | sort | uniq >> $DIR/$ANSWERS.$STRATEGY1.simplification.$t
    done
    grep -aP 'FORMULA' *.$t | grep -oP '.*(?=TECHNIQUES)' | sed "s/-$t//g" | grep -v '?' | sort | uniq > $DIR/$ANSWERS.$STRATEGY1.$t
    popd > /dev/null
    pushd $DIR2 > /dev/null
    rm -f $DIR/$ANSWERS.$STRATEGY2.sequential.$t
    rm -f $DIR/$ANSWERS.$STRATEGY2.simplification.$t
    for f in $(ls *.$t) ; do
        awk '/Sequential processing/,0' $f | grep -aP 'FORMULA' | grep -oP '.*(?=TECHNIQUES)' | sed "s/-$t//g" | grep -v '?' | sort | uniq >> $DIR/$ANSWERS.$STRATEGY2.sequential.$t
        awk '1;/Sequential processing/{exit}' $f | grep -aP 'FORMULA' | grep -oP '.*(?=TECHNIQUES)' | sed "s/-$t//g" | grep -v '?' | sort | uniq >> $DIR/$ANSWERS.$STRATEGY2.simplification.$t
    done
    grep -aP 'FORMULA' *.$t | grep -oP '.*(?=TECHNIQUES)' | sed "s/-$t//g" | grep -v '?' | sort | uniq > $DIR/$ANSWERS.$STRATEGY2.$t
    popd > /dev/null

    LEFT=$(comm -13 $ANSWERS.$STRATEGY1.$t $ANSWERS.$STRATEGY2.$t)
    RIGHT=$(comm -13 $ANSWERS.$STRATEGY2.$t $ANSWERS.$STRATEGY1.$t)
    LEFT_SEQ=$(comm -13 $ANSWERS.$STRATEGY1.sequential.$t $ANSWERS.$STRATEGY2.sequential.$t)
    RIGHT_SEQ=$(comm -13 $ANSWERS.$STRATEGY2.sequential.$t $ANSWERS.$STRATEGY1.sequential.$t)
    LEFT_SIMP=$(comm -13 $ANSWERS.$STRATEGY1.simplification.$t $ANSWERS.$STRATEGY2.simplification.$t)
    RIGHT_SIMP=$(comm -13 $ANSWERS.$STRATEGY2.simplification.$t $ANSWERS.$STRATEGY1.simplification.$t)

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
    unique_answers1_seq=$(comm -13 <(echo "$LEFT_SEQ" | grep -oP '^FORMULA [^ ]+' | sort) <(echo "$RIGHT_SEQ"  | grep -oP '^FORMULA [^ ]+' | sort))
    unique_answers2_seq=$(comm -23 <(echo "$LEFT_SEQ" | grep -oP '^FORMULA [^ ]+' | sort) <(echo "$RIGHT_SEQ"  | grep -oP '^FORMULA [^ ]+' | sort))
    unique_answers1_simp=$(comm -13 <(echo "$LEFT_SIMP" | grep -oP '^FORMULA [^ ]+' | sort) <(echo "$RIGHT_SIMP"  | grep -oP '^FORMULA [^ ]+' | sort))
    unique_answers2_simp=$(comm -23 <(echo "$LEFT_SIMP" | grep -oP '^FORMULA [^ ]+' | sort) <(echo "$RIGHT_SIMP"  | grep -oP '^FORMULA [^ ]+' | sort))

    echo "Number of unique answers given by $STRATEGY1: $(echo "$unique_answers1" | wc -l) (simplification: $(echo "$unique_answers1_simp" | wc -l), sequential: $(echo "$unique_answers1_seq" | wc -l))"
    echo "Number of unique answers given by $STRATEGY2: $(echo "$unique_answers2" | wc -l) (simplification: $(echo "$unique_answers2_simp" | wc -l), sequential: $(echo "$unique_answers2_seq" | wc -l))"

    C1=$(cat $ANSWERS.$STRATEGY1.$t | wc -l)
    C2=$(cat $ANSWERS.$STRATEGY2.$t | wc -l)
    C1_SEQ=$(cat $ANSWERS.$STRATEGY1.sequential.$t | wc -l)
    C2_SEQ=$(cat $ANSWERS.$STRATEGY2.sequential.$t | wc -l)
    C1_SIMP=$(cat $ANSWERS.$STRATEGY1.simplification.$t | wc -l)
    C2_SIMP=$(cat $ANSWERS.$STRATEGY2.simplification.$t | wc -l)
    echo "$MODELS.$BINARY.$STRATEGY1.$t -> $C1 queries answered (simplification: $C1_SIMP, sequential: $C1_SEQ)"
    echo "$MODELS.$BINARY.$STRATEGY2.$t -> $C2 queries answered (simplification: $C2_SIMP, sequential: $C2_SEQ)"
done