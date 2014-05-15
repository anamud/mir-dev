#!/bin/bash

APP=fib
TASKS=ol_fib_0,ol_fib_1,ol_fib_2
CALLED_FUNCS=fib,fib_seq
MIR_CONF="-w=1 -i -g -p -l=32"
OPF="study"
PROCESS_TASK_GRAPH=1

INPUT="48 8"

# Profile and generate data
. ${MIR_ROOT}/scripts/task-graph/stub-profile.sh

# Generate recorder data
MIR_CONF="-r -l=32" ./${APP}-prof ${INPUT}
INPUT_SS=${INPUT// /-}
${MIR_ROOT}/scripts/mirtoparaver.py *-config.rec ${APP}-${OPF}-${INPUT_SS}
mv *.prv *.pcf *.rec ${LATEST_OUTDIR} 
echo MIR_CONF=${MIR_CONF} > ${APP}-${OPF}.conf
echo INPUT=${INPUT} >> ${APP}-${OPF}.conf
cat SConstruct >> ${APP}-${OPF}.conf
mv ${APP}-${OPF}.conf ${LATEST_OUTDIR}

# Generate execution time data
echo "Profile executable" >> ${APP}-${OPF}-${INPUT_SS}-exec-time.info
MIR_CONF="-l=32" ./${APP}-prof ${INPUT} >> ${APP}-${OPF}-${INPUT_SS}-exec-time.info
echo "Optimized executable" >> ${APP}-${OPF}-${INPUT_SS}-exec-time.info
MIR_CONF="-l=32" ./${APP}-opt ${INPUT} >> ${APP}-${OPF}-${INPUT_SS}-exec-time.info
mv ${APP}-${OPF}-${INPUT_SS}-exec-time.info ${LATEST_OUTDIR}

INPUT="48 10"

# Profile and generate data
. ${MIR_ROOT}/scripts/task-graph/stub-profile.sh

# Generate recorder data
MIR_CONF="-r -l=32" ./${APP}-prof ${INPUT}
INPUT_SS=${INPUT// /-}
${MIR_ROOT}/scripts/mirtoparaver.py *-config.rec ${APP}-${OPF}-${INPUT_SS}
mv *.prv *.pcf *.rec ${LATEST_OUTDIR} 
echo MIR_CONF=${MIR_CONF} > ${APP}-${OPF}.conf
echo INPUT=${INPUT} >> ${APP}-${OPF}.conf
cat SConstruct >> ${APP}-${OPF}.conf
mv ${APP}-${OPF}.conf ${LATEST_OUTDIR}

# Generate execution time data
echo "Profile executable" >> ${APP}-${OPF}-${INPUT_SS}-exec-time.info
MIR_CONF="-l=32" ./${APP}-prof ${INPUT} >> ${APP}-${OPF}-${INPUT_SS}-exec-time.info
echo "Optimized executable" >> ${APP}-${OPF}-${INPUT_SS}-exec-time.info
MIR_CONF="-l=32" ./${APP}-opt ${INPUT} >> ${APP}-${OPF}-${INPUT_SS}-exec-time.info
mv ${APP}-${OPF}-${INPUT_SS}-exec-time.info ${LATEST_OUTDIR}

INPUT="48 12"

# Profile and generate data
. ${MIR_ROOT}/scripts/task-graph/stub-profile.sh

# Generate recorder data
MIR_CONF="-r -l=32" ./${APP}-prof ${INPUT}
INPUT_SS=${INPUT// /-}
${MIR_ROOT}/scripts/mirtoparaver.py *-config.rec ${APP}-${OPF}-${INPUT_SS}
mv *.prv *.pcf *.rec ${LATEST_OUTDIR} 
echo MIR_CONF=${MIR_CONF} > ${APP}-${OPF}.conf
echo INPUT=${INPUT} >> ${APP}-${OPF}.conf
cat SConstruct >> ${APP}-${OPF}.conf
mv ${APP}-${OPF}.conf ${LATEST_OUTDIR}

# Generate execution time data
echo "Profile executable" >> ${APP}-${OPF}-${INPUT_SS}-exec-time.info
MIR_CONF="-l=32" ./${APP}-prof ${INPUT} >> ${APP}-${OPF}-${INPUT_SS}-exec-time.info
echo "Optimized executable" >> ${APP}-${OPF}-${INPUT_SS}-exec-time.info
MIR_CONF="-l=32" ./${APP}-opt ${INPUT} >> ${APP}-${OPF}-${INPUT_SS}-exec-time.info
mv ${APP}-${OPF}-${INPUT_SS}-exec-time.info ${LATEST_OUTDIR}

INPUT="48 14"

# Profile and generate data
. ${MIR_ROOT}/scripts/task-graph/stub-profile.sh

# Generate recorder data
MIR_CONF="-r -l=32" ./${APP}-prof ${INPUT}
INPUT_SS=${INPUT// /-}
${MIR_ROOT}/scripts/mirtoparaver.py *-config.rec ${APP}-${OPF}-${INPUT_SS}
mv *.prv *.pcf *.rec ${LATEST_OUTDIR} 
echo MIR_CONF=${MIR_CONF} > ${APP}-${OPF}.conf
echo INPUT=${INPUT} >> ${APP}-${OPF}.conf
cat SConstruct >> ${APP}-${OPF}.conf
mv ${APP}-${OPF}.conf ${LATEST_OUTDIR}

# Generate execution time data
echo "Profile executable" >> ${APP}-${OPF}-${INPUT_SS}-exec-time.info
MIR_CONF="-l=32" ./${APP}-prof ${INPUT} >> ${APP}-${OPF}-${INPUT_SS}-exec-time.info
echo "Optimized executable" >> ${APP}-${OPF}-${INPUT_SS}-exec-time.info
MIR_CONF="-l=32" ./${APP}-opt ${INPUT} >> ${APP}-${OPF}-${INPUT_SS}-exec-time.info
mv ${APP}-${OPF}-${INPUT_SS}-exec-time.info ${LATEST_OUTDIR}

INPUT="48 16"

# Profile and generate data
. ${MIR_ROOT}/scripts/task-graph/stub-profile.sh

# Generate recorder data
MIR_CONF="-r -l=32" ./${APP}-prof ${INPUT}
INPUT_SS=${INPUT// /-}
${MIR_ROOT}/scripts/mirtoparaver.py *-config.rec ${APP}-${OPF}-${INPUT_SS}
mv *.prv *.pcf *.rec ${LATEST_OUTDIR} 
echo MIR_CONF=${MIR_CONF} > ${APP}-${OPF}.conf
echo INPUT=${INPUT} >> ${APP}-${OPF}.conf
cat SConstruct >> ${APP}-${OPF}.conf
mv ${APP}-${OPF}.conf ${LATEST_OUTDIR}

# Generate execution time data
echo "Profile executable" >> ${APP}-${OPF}-${INPUT_SS}-exec-time.info
MIR_CONF="-l=32" ./${APP}-prof ${INPUT} >> ${APP}-${OPF}-${INPUT_SS}-exec-time.info
echo "Optimized executable" >> ${APP}-${OPF}-${INPUT_SS}-exec-time.info
MIR_CONF="-l=32" ./${APP}-opt ${INPUT} >> ${APP}-${OPF}-${INPUT_SS}-exec-time.info
mv ${APP}-${OPF}-${INPUT_SS}-exec-time.info ${LATEST_OUTDIR}
