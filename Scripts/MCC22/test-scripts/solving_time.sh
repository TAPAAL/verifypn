#!/bin/bash

STRATEGY="$1"
MODELS="$2"
BINARY="$3"
DIR="output/$STRATEGY/$MODELS/$BINARY"
OUTPUT="time.$MODELS.$BINARY.$STRATEGY"
OUTPUT_CAR="$OUTPUT.ReachabilityCardinality.txt"
OUTPUT_FIR="$OUTPUT.ReachabilityFireability.txt"
TMP_CAR="tmp.ReachabilityCardinality.txt"
TMP_FIR="tmp.ReachabilityFireability.txt"

# for all files in $DIR
for f in $(ls $DIR) ; do
    # grep the time spent on verification
    if [[ $f == *ReachabilityCardinality* ]] ; then
        grep -oP '(?<=Spent ).[0-9\.]*(?= on verification)' $DIR/$f >> $TMP_CAR
    else
        grep -oP '(?<=Spent ).[0-9\.]*(?= on verification)' $DIR/$f >> $TMP_FIR
    fi
done

# sort the results
sort -n $TMP_CAR > $OUTPUT_CAR
sort -n $TMP_FIR > $OUTPUT_FIR

# remove the temporary file
rm $TMP_CAR $TMP_FIR