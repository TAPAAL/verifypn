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
export TEMPDIR=/scratch/malleek-16-05-23/
export SEARCH_STRATEGY=$5

mkdir -p ComparisonSearch/output/$5/$1/$2/

F="ComparisonSearch/output/$5/$1/$2/${3}.${4}"
if [ -s "$F" ]
then
    echo "No Redo!"
else
    R=$($PREFIX/run_seq_search.sh)
    echo "$R" 2> /dev/null > "$F"
fi