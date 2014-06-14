#!/bin/bash

# Note before:
# This is a generic stub. 
# Inputs (such as TASK,CALLED_FUNCS,APP,INPUT,MIR_CONF,OPF) come from application to be profiled.

# Check if PIN_ROOT is set
${PIN_ROOT:?"PIN_ROOT is not set! Aborting!"}

# Build once
scons

# Profile tasks of application
echo "Profiling tasks of application ${APP} using input ${INPUT} ..."
LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$PIN_ROOT/intel64/runtime \
MIR_CONF=${MIR_CONF} \
$PIN_ROOT/intel64/bin/pinbin \
-t ${MIR_ROOT}/scripts/task-graph/obj-intel64/mir_outline_function_profiler.so \
-o ${APP}_${OPF} \
-s ${TASKS} \
-c ${CALLED_FUNCS} \
-- ./${APP}-prof ${INPUT}

# Give the task graph an appropriate name
if [[ "${MIR_CONF}" == *-g* ]]
then
if [ -f mir-task-graph ];
then
    mv mir-task-graph ${APP}_${OPF}-fork_join_task_graph
fi
fi

# Rename mir-stats
if [[ "${MIR_CONF}" == *-i* ]]
then
if [ -f mir-stats ];
then
    mv mir-stats ${APP}_${OPF}-execution_stats
fi
fi

if [ ${PROCESS_TASK_GRAPH} -eq 1 ]
then
    # Summarize fork join task graph
    echo "Summarizing fork join task graph ..."
    Rscript ${MIR_ROOT}/scripts/task-graph/mir-fork-join-graph-info.R ${APP}_${OPF}-fork_join_task_graph 
    # Plot fork join task graph 
    echo "Plotting fork join task graph ..."
    Rscript ${MIR_ROOT}/scripts/task-graph/mir-fork-join-graph-plot.R ${APP}_${OPF}-fork_join_task_graph color
    # Annotate fork join task graph with profiling information
    echo "Annotating fork join task graph ..."
    Rscript ${MIR_ROOT}/scripts/task-graph/mir-annotate-graph.R ${APP}_${OPF}-fork_join_task_graph ${APP}_${OPF}-call_graph ${APP}_${OPF}
    # Plot annotated task graph 
    echo "Plotting annotated task graph ..."
    Rscript ${MIR_ROOT}/scripts/task-graph/mir-annotated-graph-plot.R ${APP}_${OPF}-annotated_task_graph color
else
    echo "Not processing the task graph!"
fi

# Copy files
max=`ls -1d prof_results_${OPF}_* | sed -e "s/prof_results_${OPF}_//g" | tr -dc '[0-9\n]' | sort -k 1,1n | tail -1`
mkdir prof_results_${OPF}_$((max + 1))
echo "Copying profile information to prof_results_"${OPF}_$((max + 1))
mv ${APP}_${OPF}-* prof_results_${OPF}_$((max + 1))
cp profile-${OPF}.sh prof_results_${OPF}_$((max + 1))

# Set context for upper level
LATEST_OUTDIR=prof_results_${OPF}_$((max + 1))

echo "Done!"

