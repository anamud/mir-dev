#!/bin/bash

APP=fib
OUTLINE_FUNCTIONS=ol_fib_0,ol_fib_1,ol_fib_2
CALLED_FUNCTIONS=fib,fib_seq
MIR_CONF="-w=1 -i -g -p -l=32"
OFILE_PREFIX="study"
PROCESS_PROFILE_DATA=0


function study ()
{
# Profile and generate data
. ${MIR_ROOT}/programs/common/stub-profile.sh

# Generate recorder data
MIR_CONF="-r -l=32" ./${APP}-prof ${INPUT}
INPUT_SS=${INPUT// /-}
mv *.rec ${LATEST_OUTDIR} 
cp SConstruct ${LATEST_OUTDIR}

# Generate execution time data
echo "Profile executable" >> exec-time.info
for i in `seq 1 10`;
do
MIR_CONF="-l=32" ./${APP}-prof ${INPUT} >> exec-time.info
done    
echo "Optimized executable" >> exec-time.info
for i in `seq 1 10`;
do
MIR_CONF="-l=32" ./${APP}-opt ${INPUT} >> exec-time.info
done    
mv exec-time.info ${LATEST_OUTDIR}
}

INPUT="48 8"
study

INPUT="48 12"
study

INPUT="48 16"
study
