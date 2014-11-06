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
-t ${MIR_ROOT}/scripts/profiling/task/obj-intel64/mir-of-profiler.so \
-o ${OFILE_PREFIX} \
-s ${OUTLINE_FUNCTIONS} \
-c ${CALLED_FUNCTIONS} \
-- ./${APP}-prof ${INPUT}

# Rename forks-joins file
if [[ "${MIR_CONF}" == *-g* ]]
then
if [ -f mir-forks-joins ];
then
    mv mir-forks-joins ${OFILE_PREFIX}-forks-joins
fi
fi

# Rename execution stats file
if [[ "${MIR_CONF}" == *-i* ]]
then
if [ -f mir-execution-stats ];
then
    mv mir-execution-stats ${OFILE_PREFIX}-execution-stats
fi
fi

if [ ${PROCESS_PROFILE_DATA} -eq 1 ]
then
    # Summarize fork join info 
    echo "Summarizing fork join info ..."
    Rscript ${MIR_ROOT}/scripts/profiling/task/fork-join-summary.R ${OFILE_PREFIX}-forks-joins
    # Plot fork join task graph 
    echo "Plotting fork join task graph ..."
    Rscript ${MIR_ROOT}/scripts/profiling/task/fork-join-graph-plot.R ${OFILE_PREFIX}-forks-joins color
    # Gather task performance data
    echo "Gathering task performance ..."
    Rscript ${MIR_ROOT}/scripts/profiling/task/gather-task-performance.R ${OFILE_PREFIX}-forks-joins ${OFILE_PREFIX}-instructions ${OFILE_PREFIX}-task-perf
    # Plot annotated task graph 
    echo "Plotting annotated task graph ..."
    Rscript ${MIR_ROOT}/scripts/profiling/task/annotated-graph-plot.R ${OFILE_PREFIX}-task-perf color
    # Plot tree task graph 
    echo "Plotting tree task graph ..."
    Rscript ${MIR_ROOT}/scripts/profiling/task/tree-graph-plot.R ${OFILE_PREFIX}-task-perf color
else
    echo "Not processing profile data!"
fi

# Copy files
max=`ls -1d prof_results_${OFILE_PREFIX}_* | sed -e "s/prof_results_${OFILE_PREFIX}_//g" | tr -dc '[0-9\n]' | sort -k 1,1n | tail -1`
mkdir prof_results_${OFILE_PREFIX}_$((max + 1))
echo "Copying profile information to prof_results_"${OFILE_PREFIX}_$((max + 1))
mv ${OFILE_PREFIX}-* prof_results_${OFILE_PREFIX}_$((max + 1))
cp $BASH_SOURCE prof_results_${OFILE_PREFIX}_$((max + 1))

# Set context for upper level
LATEST_OUTDIR=prof_results_${OFILE_PREFIX}_$((max + 1))

echo "Done!"

