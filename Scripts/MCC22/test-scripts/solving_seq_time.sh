#!/bin/bash

MODELS="$1"
BINARY="$2"
DIR="BENCHKIT/$MODELS/$BINARY/"
OUTPUT="answers/time/$MODELS.$BINARY.seq"
OUTPUT_CAR="$OUTPUT.ReachabilityCardinality.txt"
OUTPUT_FIR="$OUTPUT.ReachabilityFireability.txt"
TMP_CAR="tmp.ReachabilityCardinality.txt"
TMP_FIR="tmp.ReachabilityFireability.txt"

# for all files in $DIR
for f in $(ls $DIR) ; do
    # grep the time spent on verification
    if [[ $f == *ReachabilityCardinality* ]] ; then
        awk '/Step 2: Sequential processing/,/Step 3|Step 4/' $DIR/$f | grep -B 3 'RANDOMWALK' | grep -oP '(?<=Spent ).[0-9\.e\-]*(?= on verification)' | uniq >> $TMP_CAR
    else
        awk '/Step 2: Sequential processing/,/Step 3|Step 4/' $DIR/$f | grep -B 3 'RANDOMWALK' | grep -oP '(?<=Spent ).[0-9\.e\-]*(?= on verification)' | uniq >> $TMP_FIR
    fi
done

# sort the results
sort -n $TMP_CAR > $OUTPUT_CAR
sort -n $TMP_FIR > $OUTPUT_FIR

# remove the temporary file
rm $TMP_CAR $TMP_FIR