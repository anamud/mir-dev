#!/bin/bash

APP=fib
OUTLINE_FUNCTIONS=ol_fib_0,ol_fib_1,ol_fib_2
CALLED_FUNCTIONS=fib,fib_seq
MIR_CONF="-w=1 -i -g -p -l=32"
OFILE_PREFIX="study"
PROCESS_PROFILE_DATA=1

INPUT="48 8"

# Profile and generate data
. ${MIR_ROOT}/programs/common/stub-profile.sh

# Generate recorder data
MIR_CONF="-r -l=32" ./${APP}-prof ${INPUT}
INPUT_SS=${INPUT// /-}
${MIR_ROOT}/scripts/mirtoparaver.py *-config.rec ${APP}-${OFILE_PREFIX}-${INPUT_SS}
mv *.prv *.pcf *.rec ${LATEST_OUTDIR} 
echo MIR_CONF=${MIR_CONF} > ${APP}-${OFILE_PREFIX}.conf
echo INPUT=${INPUT} >> ${APP}-${OFILE_PREFIX}.conf
cat SConstruct >> ${APP}-${OFILE_PREFIX}.conf
mv ${APP}-${OFILE_PREFIX}.conf ${LATEST_OUTDIR}

# Generate execution time data
echo "Profile executable" >> ${APP}-${OFILE_PREFIX}-${INPUT_SS}-exec-time.info
MIR_CONF="-l=32" ./${APP}-prof ${INPUT} >> ${APP}-${OFILE_PREFIX}-${INPUT_SS}-exec-time.info
echo "Optimized executable" >> ${APP}-${OFILE_PREFIX}-${INPUT_SS}-exec-time.info
MIR_CONF="-l=32" ./${APP}-opt ${INPUT} >> ${APP}-${OFILE_PREFIX}-${INPUT_SS}-exec-time.info
mv ${APP}-${OFILE_PREFIX}-${INPUT_SS}-exec-time.info ${LATEST_OUTDIR}

INPUT="48 10"

# Profile and generate data
. ${MIR_ROOT}/programs/common/stub-profile.sh

# Generate recorder data
MIR_CONF="-r -l=32" ./${APP}-prof ${INPUT}
INPUT_SS=${INPUT// /-}
${MIR_ROOT}/scripts/mirtoparaver.py *-config.rec ${APP}-${OFILE_PREFIX}-${INPUT_SS}
mv *.prv *.pcf *.rec ${LATEST_OUTDIR} 
echo MIR_CONF=${MIR_CONF} > ${APP}-${OFILE_PREFIX}.conf
echo INPUT=${INPUT} >> ${APP}-${OFILE_PREFIX}.conf
cat SConstruct >> ${APP}-${OFILE_PREFIX}.conf
mv ${APP}-${OFILE_PREFIX}.conf ${LATEST_OUTDIR}

# Generate execution time data
echo "Profile executable" >> ${APP}-${OFILE_PREFIX}-${INPUT_SS}-exec-time.info
MIR_CONF="-l=32" ./${APP}-prof ${INPUT} >> ${APP}-${OFILE_PREFIX}-${INPUT_SS}-exec-time.info
echo "Optimized executable" >> ${APP}-${OFILE_PREFIX}-${INPUT_SS}-exec-time.info
MIR_CONF="-l=32" ./${APP}-opt ${INPUT} >> ${APP}-${OFILE_PREFIX}-${INPUT_SS}-exec-time.info
mv ${APP}-${OFILE_PREFIX}-${INPUT_SS}-exec-time.info ${LATEST_OUTDIR}

INPUT="48 12"

# Profile and generate data
. ${MIR_ROOT}/programs/common/stub-profile.sh

# Generate recorder data
MIR_CONF="-r -l=32" ./${APP}-prof ${INPUT}
INPUT_SS=${INPUT// /-}
${MIR_ROOT}/scripts/mirtoparaver.py *-config.rec ${APP}-${OFILE_PREFIX}-${INPUT_SS}
mv *.prv *.pcf *.rec ${LATEST_OUTDIR} 
echo MIR_CONF=${MIR_CONF} > ${APP}-${OFILE_PREFIX}.conf
echo INPUT=${INPUT} >> ${APP}-${OFILE_PREFIX}.conf
cat SConstruct >> ${APP}-${OFILE_PREFIX}.conf
mv ${APP}-${OFILE_PREFIX}.conf ${LATEST_OUTDIR}

# Generate execution time data
echo "Profile executable" >> ${APP}-${OFILE_PREFIX}-${INPUT_SS}-exec-time.info
MIR_CONF="-l=32" ./${APP}-prof ${INPUT} >> ${APP}-${OFILE_PREFIX}-${INPUT_SS}-exec-time.info
echo "Optimized executable" >> ${APP}-${OFILE_PREFIX}-${INPUT_SS}-exec-time.info
MIR_CONF="-l=32" ./${APP}-opt ${INPUT} >> ${APP}-${OFILE_PREFIX}-${INPUT_SS}-exec-time.info
mv ${APP}-${OFILE_PREFIX}-${INPUT_SS}-exec-time.info ${LATEST_OUTDIR}

INPUT="48 14"

# Profile and generate data
. ${MIR_ROOT}/programs/common/stub-profile.sh

# Generate recorder data
MIR_CONF="-r -l=32" ./${APP}-prof ${INPUT}
INPUT_SS=${INPUT// /-}
${MIR_ROOT}/scripts/mirtoparaver.py *-config.rec ${APP}-${OFILE_PREFIX}-${INPUT_SS}
mv *.prv *.pcf *.rec ${LATEST_OUTDIR} 
echo MIR_CONF=${MIR_CONF} > ${APP}-${OFILE_PREFIX}.conf
echo INPUT=${INPUT} >> ${APP}-${OFILE_PREFIX}.conf
cat SConstruct >> ${APP}-${OFILE_PREFIX}.conf
mv ${APP}-${OFILE_PREFIX}.conf ${LATEST_OUTDIR}

# Generate execution time data
echo "Profile executable" >> ${APP}-${OFILE_PREFIX}-${INPUT_SS}-exec-time.info
MIR_CONF="-l=32" ./${APP}-prof ${INPUT} >> ${APP}-${OFILE_PREFIX}-${INPUT_SS}-exec-time.info
echo "Optimized executable" >> ${APP}-${OFILE_PREFIX}-${INPUT_SS}-exec-time.info
MIR_CONF="-l=32" ./${APP}-opt ${INPUT} >> ${APP}-${OFILE_PREFIX}-${INPUT_SS}-exec-time.info
mv ${APP}-${OFILE_PREFIX}-${INPUT_SS}-exec-time.info ${LATEST_OUTDIR}

INPUT="48 16"

# Profile and generate data
. ${MIR_ROOT}/programs/common/stub-profile.sh

# Generate recorder data
MIR_CONF="-r -l=32" ./${APP}-prof ${INPUT}
INPUT_SS=${INPUT// /-}
${MIR_ROOT}/scripts/mirtoparaver.py *-config.rec ${APP}-${OFILE_PREFIX}-${INPUT_SS}
mv *.prv *.pcf *.rec ${LATEST_OUTDIR} 
echo MIR_CONF=${MIR_CONF} > ${APP}-${OFILE_PREFIX}.conf
echo INPUT=${INPUT} >> ${APP}-${OFILE_PREFIX}.conf
cat SConstruct >> ${APP}-${OFILE_PREFIX}.conf
mv ${APP}-${OFILE_PREFIX}.conf ${LATEST_OUTDIR}

# Generate execution time data
echo "Profile executable" >> ${APP}-${OFILE_PREFIX}-${INPUT_SS}-exec-time.info
MIR_CONF="-l=32" ./${APP}-prof ${INPUT} >> ${APP}-${OFILE_PREFIX}-${INPUT_SS}-exec-time.info
echo "Optimized executable" >> ${APP}-${OFILE_PREFIX}-${INPUT_SS}-exec-time.info
MIR_CONF="-l=32" ./${APP}-opt ${INPUT} >> ${APP}-${OFILE_PREFIX}-${INPUT_SS}-exec-time.info
mv ${APP}-${OFILE_PREFIX}-${INPUT_SS}-exec-time.info ${LATEST_OUTDIR}
