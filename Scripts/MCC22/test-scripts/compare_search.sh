#!/bin/bash

BIN="$1"
SEARCH_STRATEGY="$2"
F=MCC2022
SCRIPTS=MCC22-2cee3b71

# compare_search.sh appelle ./run_job.sh avec $1=MCC22-2cee3b71 $2=binary_filename $3=modelFolder $4=Reachability{Cardinality,Fireability} $5=SearchStrategy
# run_job.sh appelle ./ComparisonSearch/$1/run_seq_search.sh et l'écrit dans ComparisonSearch/output/$1/$2/$5/${3}.${4}

for t in Reachability{Cardinality,Fireability} ; do # For each type of property
    for i in $(ls $F ) ; do # For each model
        # 1 job that runs sequentially all the queries of a model with 5 minutes timeout
        # for one type of search (RANDOMWALK)
        sbatch --partition=naples --job-name="$t($BIN):$i" -n 1 -c 4  ./run_job.sh $SCRIPTS $BIN $i $t $SEARCH_STRATEGY
    done
done