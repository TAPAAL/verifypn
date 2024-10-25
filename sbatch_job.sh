#!/bin/bash
#SBATCH --mail-type=ALL  # Type of email notification: BEGIN,END,FAIL,ALL
#SBATCH --mail-user=jhajri20@student.aau.dk
#SBATCH --output=/nfs/home/student.aau.dk/jhajri20/slurm-output/test-runner-%A_%a.out  # Redirect the output stream to this file (%A_%a is the job's array-id and index)
#SBATCH --error=/nfs/home/student.aau.dk/jhajri20/slurm-output/test-runner-%A_%a.err   # Redirect the error stream to this file (%A_%a is the job's array-id and index)
#SBATCH --partition=naples,dhabi,rome  # If you need run-times to be consistent across tests, you may need to restrict to one partition.
#SBATCH --mem=16G  # Memory limit that slurm allocates
#SBATCH --time=2:00:00
#SBATCH --ntasks=1
#SBATCH --cpus-per-task=64

let "m=1024*1024*16"
ulimit -v $m

U=$(whoami)
PD=$(pwd)
# Create a unique folder for this job execution in your scratch folder. (In case your program writes temporary files.)
SCRATCH_DIRECTORY=/scratch/${U}/${SLURM_JOBID}
mkdir -p ${SCRATCH_DIRECTORY}
cd ${SCRATCH_DIRECTORY}

mkdir results
cp -R /nfs/petrinet/mcc/2024/colour/ .
cp /nfs/home/student.aau.dk/jhajri20/verifypn/verifypn verifypn

chmod u+x verifypn

python3 /nfs/home/student.aau.dk/jhajri20/verifypn/mcc_tester.py -m colour/ -t 10 -n 64 -o results/results.csv -b verifypn

cp results/results.csv /nfs/home/student.aau.dk/jhajri20/verifypn/results.csv

cd /scratch/${U}
[ -d "${SLURM_JOBID}" ] && rm -r ${SLURM_JOBID}
