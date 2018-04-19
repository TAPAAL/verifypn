#!/bin/bash

#ENDING="UpperBounds"
RESULT_VECTOR=${RESULT_VECTOR:-"0 1 10 11 12 13 14 15 2 3 4 5 6 7 8 9"}
RESULT_FILE=${RESULT_FILE:-"mcc17.csv"}

TMP=$(mktemp ./results2csv.tmp.XXXXXX)

# Testing of awk arrays, but kept for nice printing of result vector
awk -v rv="$RESULT_VECTOR" 'BEGIN { print "Building results for vector:"; 
    split(rv,vec); for (i = 1; i <= length(vec); i++) printf vec[i] " "; print "" }'

echo "Input,results" > $TMP

for f in $1/*.{ReachabilityDeadlock,ReachabilityCardinality,ReachabilityFireability,CTLCardinality,CTLFireability,UpperBounds}; do
    fn=$(basename $f)
    ending=${fn#*.}
    echo -n "${fn%.*},${ending}," >> $TMP
    grep FORM $f | awk -v rv="$RESULT_VECTOR" -v type="$ending" '{ data[v[split($2,v,"-")]]=$3 }; END { split(rv,vec); for (i=1;i<=length(vec);i++) printf (length(data[vec[i]]) == 0)? "? " : (type=="UpperBounds" ? data[vec[i]] " " : substr(data[vec[i]],0,1) " ") }' | sed -e 's/[[:space:]]*$//' >> $TMP
    echo "" >> $TMP
done

./compare_csv.awk "$RESULT_FILE" "$TMP"

rm $TMP
