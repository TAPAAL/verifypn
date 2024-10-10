BINNAME="verifypn-linux64"
if [ -z "$1" ] ; then
    echo "The cmake build folder needs to be specified as the first argument"
    exit 1
fi

if ! [ -f "$1/verifypn/bin/$BINNAME" ]; then
    echo "build file does not exists, either the cmake build folder is incorrect or verifypn has not been built"
    exit 1
fi

{
    echo -mkdir verifypn
    echo -mkdir slurm-output
} | sftp -b - mcc3
scp "$1/verifypn/bin/$BINNAME" mcc3:verifypn/verifypn
scp "mcc_tester.py" mcc3:verifypn/mcc_tester.py
scp sbatch_job.sh mcc3:verifypn/sbatch_job.sh