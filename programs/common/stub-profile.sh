#!/bin/bash

# Check if PIN_ROOT is set
${PIN_ROOT:?"PIN_ROOT is not set! Aborting!"}

# Build once
scons

# Profile tasks of application
echo "Profiling tasks of application ${APP} using input ${INPUT} ..."
LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$PIN_ROOT/intel64/runtime \
MIR_CONF=${MIR_CONF} \
$PIN_ROOT/intel64/bin/pinbin \
-t ${MIR_ROOT}/scripts/profiling/task/obj-intel64/mir_of_profiler.so \
-o ${OFILE_PREFIX} \
-s ${OUTLINE_FUNCTIONS} \
-c ${CALLED_FUNCTIONS} \
-- ./${APP}-prof ${INPUT}

# Rename task stats file
if [[ "${MIR_CONF}" == *-g* ]]
then
if [ -f mir-task-stats ];
then
    mv mir-task-stats ${OFILE_PREFIX}-task-stats
fi
fi

# Rename worker stats file
if [[ "${MIR_CONF}" == *-i* ]]
then
if [ -f mir-worker-stats ];
then
    mv mir-worker-stats ${OFILE_PREFIX}-worker-stats
fi
fi

if [ ${PROCESS_PROFILE_DATA} -eq 1 ]
then
    # Summarize task stats 
    echo "Summarizing task stats ..."
    Rscript ${MIR_ROOT}/scripts/profiling/task/task-stats-summary.R ${OFILE_PREFIX}-task-stats
    # Gather task performance data
    echo "Gathering task performance ..."
    Rscript ${MIR_ROOT}/scripts/profiling/task/gather-task-performance.R ${OFILE_PREFIX}-task-stats ${OFILE_PREFIX}-instructions ${OFILE_PREFIX}-task-perf
    # Plot task graph 
    echo "Plotting series-parallel task graph ..."
    Rscript ${MIR_ROOT}/scripts/profiling/task/task-graph-plot.R -d ${OFILE_PREFIX}-task-perf -p color -o ${OFILE_PREFIX}-task-graph-ser-par
    # Plot tree task graph 
    echo "Plotting tree task graph ..."
    Rscript ${MIR_ROOT}/scripts/profiling/task/task-graph-plot.R -d ${OFILE_PREFIX}-task-perf -t -p color -o ${OFILE_PREFIX}-task-graph-tree
else
    echo "Not processing profile data!"
fi

# Settings 
echo APP=$APP > ${OFILE_PREFIX}-settings
echo OUTLINE_FUNCTIONS=$OUTLINE_FUNCTIONS >> ${OFILE_PREFIX}-settings
echo CALLED_FUNCTIONS=$CALLED_FUNCTIONS >> ${OFILE_PREFIX}-settings
echo INPUT=$INPUT >> ${OFILE_PREFIX}-settings
echo MIR_CONF=$MIR_CONF >> ${OFILE_PREFIX}-settings

# Copy files
max=`ls -1d prof_results_${OFILE_PREFIX}_* | sed -e "s/prof_results_${OFILE_PREFIX}_//g" | tr -dc '[0-9\n]' | sort -k 1,1n | tail -1`
mkdir prof_results_${OFILE_PREFIX}_$((max + 1))
echo "Copying profile information to prof_results_"${OFILE_PREFIX}_$((max + 1))
mv ${OFILE_PREFIX}-* prof_results_${OFILE_PREFIX}_$((max + 1))
cp $BASH_SOURCE prof_results_${OFILE_PREFIX}_$((max + 1))

# Set context for upper level
LATEST_OUTDIR=prof_results_${OFILE_PREFIX}_$((max + 1))

echo "Done!"

