#!/bin/bash

BIN="$1"
USEREDUCTION="$2" # 0 or 1

F=MCC2022
SCRIPTS=MCC22-2cee3b71

for t in Reachability{Cardinality,Fireability} ; do # For each type of property
    for i in $(ls $F ) ; do # For each model
        for maxsteps in 5000 10000 20000 40000 80000 ; do
            # 1 job that runs sequentially all the queries of a model with 5 minutes timeout
            # for one type of search (RANDOMWALK) and one maxsteps
            sbatch --partition=naples --job-name="$t($BIN):$i($maxsteps steps)" -n 1 -c 4  ./run_job_maxsteps.sh $SCRIPTS $BIN $i $t $maxsteps $USEREDUCTION
        done
    done
done