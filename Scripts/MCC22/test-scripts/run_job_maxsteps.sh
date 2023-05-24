#!/bin/bash
#SBATCH --time=1:30:00
#SBATCH --mail-user=malodautry@cs.aau.dk
#SBATCH --mail-type=FAIL
#SBATCH --mem=16G
#SBATCH --output=/dev/null

export PREFIX=./ComparisonSearch/$1
export VERIFYPN=$(pwd)/binaries/$2
export MODEL_PATH=$(pwd)/MCC2022/$3
export BK_EXAMINATION=$4
export BK_TIME_CONFINEMENT=5100 # 16 queries * 5 minutes + 5 minutes
export TEMPDIR=/scratch/malleek-22-05-23/
export SEARCH_STRATEGY=RANDOMWALK
export MAXSTEPS=$5
export USEREDUCTION=$6
DIR=ComparisonSearch/output/$SEARCH_STRATEGY/MaxSteps_$MAXSTEPS/$1/$2

mkdir -p $DIR/Reduction $DIR/NoReduction

F="$DIR/Reduction/${3}.${4}"
if [ "$USEREDUCTION" -eq "0" ] ; then
    F="$DIR/NoReduction/${3}.${4}"
fi
if [ -s "$F" ]
then
    echo "No Redo!"
else
    R=$($PREFIX/run_seq_search_maxsteps.sh)
    echo "$R" 2> /dev/null > "$F"
fi