#!/bin/bash
#SBATCH --time=1:05:00
#SBATCH --mail-user=srba@cs.aau.dk
#SBATCH --mail-type=FAIL
#SBATCH --mem_bind=verbose,local
#SBATCH --partition=production

let "m=1024*1024*15"
ulimit -v $m


export BK_TOOL=tapaal
export PREFIX=./scripts/$1
export VERIFYPN=./binaries/$2
export MODEL_PATH=./$5/$3
export BK_EXAMINATION=$4

mkdir -p BENCHKIT/$1/$2/
echo "$PREFIX/BenchKit_head.sh &> BENCHKIT/$1/$2/${3}.${4}"

$PREFIX/BenchKit_head.sh &> BENCHKIT/$1/$2/${3}.${4}

