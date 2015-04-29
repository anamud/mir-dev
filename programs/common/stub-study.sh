function prolog ()
{
echo "In prolog"

# Build once
scons

# Take timestamp
touch timestamp
}

function epilog ()
{
echo "In epilog"

# Copy output files
rm -rf $1
mkdir $1
echo "Copying output files to" $1
find . -maxdepth 1 -type f -newer timestamp -exec mv {} $1 \;
cp $BASH_SOURCE $1
cp SConstruct $1
}

function speedup ()
{
echo "In speedup"

for j in "$@";
do
outf="speedup-$j.info"
echo "# $j cores"  | tee $outf
for i in `seq 1 10`;
do
    MIR_CONF="-w=$j -s=$SCHED -l=$STACK -m=$MEMPOL" ./${PROG} ${INPUT} 2>&1 | tee -a $outf
done    
done    
}

function recorder ()
{
echo "In recorder"

outf="recorder.info"
MIR_CONF="-w=$WORKER -s=$SCHED -l=$STACK -m=$MEMPOL -r" ./${PROG} ${INPUT} 2>&1 | tee $outf
}

function task_stats ()
{
echo "In task_stats"

outf="task_stats.info"
MIR_CONF="-w=$WORKER -s=$SCHED -l=$STACK -m=$MEMPOL -g" ./${PROG} ${INPUT} 2>&1 | tee $outf
}

function worker_stats ()
{
echo "In worker_stats"

outf="worker_stats.info"
MIR_CONF="-s=$SCHED -l=$STACK -m=$MEMPOL -i" ./${PROG} ${INPUT} 2>&1 | tee $outf
}

function all_stats ()
{
echo "In all_stats"

outf="all_stats.info"
MIR_CONF="-w=$WORKER -s=$SCHED -l=$STACK -m=$MEMPOL -i -g" ./${PROG} ${INPUT} 2>&1 | tee $outf
}

function recorder_and_stats ()
{
echo "In recorder_and_stats"

outf="recorder_and_stats.info"
MIR_CONF="-w=$WORKER -s=$SCHED -l=$STACK -m=$MEMPOL -i -g -r" ./${PROG} ${INPUT} 2>&1 | tee $outf
}
