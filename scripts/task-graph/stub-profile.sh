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
-t ${MIR_ROOT}/scripts/task-graph/obj-intel64/mir_routine_trace.so \
-o ${APP}_${OPF}_call_graph \
-s ${TASKS} \
-c ${CALLED_FUNCS} \
-- ./${APP}-prof ${INPUT}

# Give the task graph an appropriate name
if [[ "${MIR_CONF}" == *-g* ]]
then
if [ -f mir-task-graph ];
then
    mv mir-task-graph ${APP}_${OPF}_task_graph
fi
fi

# Rename mir-stats
if [[ "${MIR_CONF}" == *-i* ]]
then
if [ -f mir-stats ];
then
    mv mir-stats ${APP}_${OPF}_mir-stats
fi
fi

if [ ${PROCESS_TASK_GRAPH} -eq 1 ]
then
    # Summarize task graph
    echo "Summarizing task graph (quick summary) ..."
    Rscript ${MIR_ROOT}/scripts/task-graph/mir-task-graph-info.R ${APP}_${OPF}_task_graph 
    # Bind task graph to profiled data
    echo "Binding task graph to profiled data ..."
    Rscript ${MIR_ROOT}/scripts/task-graph/mir-callgraph-taskgraph-id-bind.R ${APP}_${OPF}_task_graph ${APP}_${OPF}_call_graph.detail.csv 
    # Plotting task graph 
    echo "Plotting task graph (detailed summary) ..."
    Rscript ${MIR_ROOT}/scripts/task-graph/mir-task-graph-plot.R ${APP}_${OPF}_task_graph-ext
else
    echo "Not processing the task graph!"
fi

# Copy files
max=`ls -1d prof_results_${OPF}_* | sed -e "s/prof_results_${OPF}_//g" | tr -dc '[0-9\n]' | sort -k 1,1n | tail -1`
mkdir prof_results_${OPF}_$((max + 1))
echo "Copying profile information to prof_results_"${OPF}_$((max + 1))
mv ${APP}_${OPF}_* prof_results_${OPF}_$((max + 1))
cp profile-${OPF}.sh prof_results_${OPF}_$((max + 1))

# Set context for upper level
LATEST_OUTDIR=prof_results_${OPF}_$((max + 1))

echo "Done!"

