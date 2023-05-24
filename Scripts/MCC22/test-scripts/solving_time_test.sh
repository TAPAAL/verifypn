#!/bin/bash

STRATEGY="$1"
MODELS="$2"
BINARY="$3"
MAXSTEPS="MaxSteps_$4"
REDUCTION="$5"
DIR="output/$STRATEGY/$MAXSTEPS/$MODELS/$BINARY/$REDUCTION"
OUTPUT="time/$MODELS.$BINARY.$STRATEGY.$MAXSTEPS.$REDUCTION"
OUTPUT_CAR="test.ReachabilityCardinality.txt"
OUTPUT_FIR="test.ReachabilityFireability.txt"
TMP_CAR="tmp.ReachabilityCardinality.txt"
TMP_FIR="tmp.ReachabilityFireability.txt"

# for all files in $DIR
for f in $(ls $DIR) ; do
    # grep the time spent on verification
    if [[ $f == *ReachabilityCardinality* ]] ; then
        grep -P -A 3 '(?<=Spent ).[0-9\.e\-]*(?= on verification)' $DIR/$f | grep -aP 'FORMULA' | grep -oP '.*(?=TECHNIQUES)' | sed "s/-ReachabilityCardinality//g" | grep -v '?' | sort | uniq >> $TMP_CAR
    else
        grep -P -A 3 '(?<=Spent ).[0-9\.e\-]*(?= on verification)' $DIR/$f | grep -aP 'FORMULA' | grep -oP '.*(?=TECHNIQUES)' | sed "s/-ReachabilityFireability//g" | grep -v '?' | sort | uniq >> $TMP_FIR
    fi
done

# sort the results
sort -n $TMP_CAR > $OUTPUT_CAR
sort -n $TMP_FIR > $OUTPUT_FIR

# remove the temporary file
rm $TMP_CAR $TMP_FIR