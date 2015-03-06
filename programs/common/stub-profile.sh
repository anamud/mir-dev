#!/bin/bash

# Check if PIN_ROOT is set
${PIN_ROOT:?"PIN_ROOT is not set! Aborting!"}

# Build once
scons

# Take timestamp
touch timestamp

# Profile tasks of application
echo "Profiling tasks of application ${APP} using input ${INPUT} ..."
LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$PIN_ROOT/intel64/runtime \
MIR_CONF=${MIR_CONF} \
$PIN_ROOT/intel64/bin/pinbin \
-t ${MIR_ROOT}/scripts/profiling/task/obj-intel64/mir_of_profiler.so \
-s ${OUTLINE_FUNCTIONS} \
-c ${CALLED_FUNCTIONS} \
-- ./${APP}-prof ${INPUT}

if [ ${PROCESS_PROFILE_DATA} -eq 1 ]
then
    # Process task stats 
    echo "Processing task statistics ..."
    Rscript ${MIR_ROOT}/scripts/profiling/task/process-task-stats.R -d mir-task-stats
    # Merge task performance data
    echo "Merging task performance data ..."
    Rscript ${MIR_ROOT}/scripts/profiling/task/merge-task-performance.R -l mir-task-stats.processed -r mir-ofp-instructions -k task -o merged-task-perf
    # Summarize task performance data
    echo "Summarizing task performance data ..."
    Rscript ${MIR_ROOT}/scripts/profiling/task/summarize-task-stats.R -d merged-task-perf
    # Plot task graph 
    echo "Plotting series-parallel task graph ..."
    Rscript ${MIR_ROOT}/scripts/profiling/task/plot-task-graph.R -d merged-task-perf -p color -o ser-par-task-graph
    # Plot tree task graph 
    echo "Plotting tree task graph ..."
    Rscript ${MIR_ROOT}/scripts/profiling/task/plot-task-graph.R -d merged-task-perf -t -p color -o tree-task-graph
else
    echo "Not processing profiling data!"
fi

# Settings 
echo APP=$APP > profile-settings
echo OUTLINE_FUNCTIONS=$OUTLINE_FUNCTIONS >> profile-settings  
echo CALLED_FUNCTIONS=$CALLED_FUNCTIONS >> profile-settings  
echo INPUT=$INPUT >> profile-settings 
echo MIR_CONF=$MIR_CONF >> profile-settings  

# Helper function 
# Copy files
max=`ls -1d prof_results_* | sed -e "s/prof_results_//g" | tr -dc '[0-9\n]' | sort -k 1,1n | tail -1`
outdir=prof_results_${OFILE_PREFIX}_$((max + 1))
mkdir $outdir
echo "Copying profiling data to" $outdir
find . -maxdepth 1 -type f -newer timestamp -exec mv {} $outdir \;
cp $BASH_SOURCE $outdir

# Set context for upper level
LATEST_OUTDIR=$outdir

echo "Done!"

