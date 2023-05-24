#!/bin/bash

STRATEGY=RANDOMWALK
ISREDUCTION="$1"
if [[ $ISREDUCTION == 1 ]] ; then
    REDUCTION=Reduction
else
    REDUCTION=NoReduction
fi
MAXSTEPS1="MaxSteps_$2"
MAXSTEPS2="MaxSteps_$3"
MODELS="$4"
BINARY="$5"
DIR1="output/$STRATEGY/$MAXSTEPS1/$MODELS/$BINARY/$REDUCTION"
DIR2="output/$STRATEGY/$MAXSTEPS2/$MODELS/$BINARY/$REDUCTION"
ANSWERS="answers/$MODELS.$BINARY.$STRATEGY.$REDUCTION"
DIR=$(pwd)

for t in Reachability{Cardinality,Fireability} ; do
    pushd $DIR1 > /dev/null
    rm -f $DIR/$ANSWERS.$MAXSTEPS1.sequential.$t
    rm -f $DIR/$ANSWERS.$MAXSTEPS1.simplification.$t
    for f in $(ls *.$t) ; do
        awk '/Sequential processing/,0' $f | grep -aP 'FORMULA' | grep -oP '.*(?=TECHNIQUES)' | sed "s/-$t//g" | grep -v '?' | sort | uniq >> $DIR/$ANSWERS.$MAXSTEPS1.sequential.$t
        awk '1;/Sequential processing/{exit}' $f | grep -aP 'FORMULA' | grep -oP '.*(?=TECHNIQUES)' | sed "s/-$t//g" | grep -v '?' | sort | uniq >> $DIR/$ANSWERS.$MAXSTEPS1.simplification.$t
    done
    grep -aP 'FORMULA' *.$t | grep -oP '.*(?=TECHNIQUES)' | sed "s/-$t//g" | grep -v '?' | sort | uniq > $DIR/$ANSWERS.$MAXSTEPS1.$t
    popd > /dev/null
    pushd $DIR2 > /dev/null
    rm -f $DIR/$ANSWERS.$MAXSTEPS2.sequential.$t
    rm -f $DIR/$ANSWERS.$MAXSTEPS2.simplification.$t
    for f in $(ls *.$t) ; do
        awk '/Sequential processing/,0' $f | grep -aP 'FORMULA' | grep -oP '.*(?=TECHNIQUES)' | sed "s/-$t//g" | grep -v '?' | sort | uniq >> $DIR/$ANSWERS.$MAXSTEPS2.sequential.$t
        awk '1;/Sequential processing/{exit}' $f | grep -aP 'FORMULA' | grep -oP '.*(?=TECHNIQUES)' | sed "s/-$t//g" | grep -v '?' | sort | uniq >> $DIR/$ANSWERS.$MAXSTEPS2.simplification.$t
    done
    grep -aP 'FORMULA' *.$t | grep -oP '.*(?=TECHNIQUES)' | sed "s/-$t//g" | grep -v '?' | sort | uniq > $DIR/$ANSWERS.$MAXSTEPS2.$t
    popd > /dev/null

    LEFT=$(comm -13 $ANSWERS.$MAXSTEPS1.$t $ANSWERS.$MAXSTEPS2.$t)
    RIGHT=$(comm -13 $ANSWERS.$MAXSTEPS2.$t $ANSWERS.$MAXSTEPS1.$t)
    LEFT_SEQ=$(comm -13 $ANSWERS.$MAXSTEPS1.sequential.$t $ANSWERS.$MAXSTEPS2.sequential.$t)
    RIGHT_SEQ=$(comm -13 $ANSWERS.$MAXSTEPS2.sequential.$t $ANSWERS.$MAXSTEPS1.sequential.$t)
    LEFT_SIMP=$(comm -13 $ANSWERS.$MAXSTEPS1.simplification.$t $ANSWERS.$MAXSTEPS2.simplification.$t)
    RIGHT_SIMP=$(comm -13 $ANSWERS.$MAXSTEPS2.simplification.$t $ANSWERS.$MAXSTEPS1.simplification.$t)

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

    echo "Number of unique answers given by $MAXSTEPS1: $(echo "$unique_answers1" | wc -l) (simplification: $(echo "$unique_answers1_simp" | wc -l), sequential: $(echo "$unique_answers1_seq" | wc -l))"
    echo "Number of unique answers given by $MAXSTEPS2: $(echo "$unique_answers2" | wc -l) (simplification: $(echo "$unique_answers2_simp" | wc -l), sequential: $(echo "$unique_answers2_seq" | wc -l))"

    C1=$(cat $ANSWERS.$MAXSTEPS1.$t | wc -l)
    C2=$(cat $ANSWERS.$MAXSTEPS2.$t | wc -l)
    C1_SEQ=$(cat $ANSWERS.$MAXSTEPS1.sequential.$t | wc -l)
    C2_SEQ=$(cat $ANSWERS.$MAXSTEPS2.sequential.$t | wc -l)
    C1_SIMP=$(cat $ANSWERS.$MAXSTEPS1.simplification.$t | wc -l)
    C2_SIMP=$(cat $ANSWERS.$MAXSTEPS2.simplification.$t | wc -l)
    echo "$STRATEGY.$MAXSTEPS1.$t -> $C1 queries answered (simplification: $C1_SIMP, sequential: $C1_SEQ)"
    echo "$STRATEGY.$MAXSTEPS2.$t -> $C2 queries answered (simplification: $C2_SIMP, sequential: $C2_SEQ)"
done
