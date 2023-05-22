#!/bin/bash

BIN="$1"
F=MCC2022
SCRIPTS=MCC22-2cee3b71


for t in Reachability{Cardinality,Fireability} ; do
    echo "--- $t ---"
    OUTPUT=answers/$SCRIPTS.$BIN.$t.seq
    rm -f $OUTPUT
    for i in $(ls $F ) ; do
        f="BENCHKIT/$SCRIPTS/$BIN/$i.$t"
        awk '/Step 2: Sequential processing/,/Step 3|Step 4/' $f | grep -aP 'FORMULA' | grep -oP '.*(?=TECHNIQUES)' | sed "s/-$t//g" | grep -v '?' | sort | uniq >> $OUTPUT
    done
    echo "$(cat "$OUTPUT" | wc -l ) queries answered by sequential processing"
done