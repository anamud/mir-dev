#!/bin/bash

# Note before:
# This is a generic stub. 
# Inputs (such as TASK,CALLED_FUNCS,APP,INPUT,MIR_CONF,OPF) come from application to be profiled.

# Check if PIN_ROOT is set
${PIN_ROOT:?"PIN_ROOT is not set! Aborting!"}

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
mv mir-task-graph ${APP}_${OPF}_task_graph

if [ ${BIND_TASK_GRAPH} -eq 1 ]
then
    # Bind task graph to profiled data
    echo "Binding task graph to profiled data ..."
    Rscript ${MIR_ROOT}/scripts/task-graph/mir-callgraph-taskgraph-id-bind.R ${APP}_${OPF}_task_graph ${APP}_${OPF}_call_graph.detail.csv 
else
    echo "Not binding the task graph!"
fi

if [ ${PLOT_TASK_GRAPH} -eq 1 ]
then
    # Plot task graph
    echo "Plotting task graph ..."
    Rscript ${MIR_ROOT}/scripts/task-graph/mir-task-graph-plot.R ${APP}_${OPF}_task_graph 
    #dot -T png ${APP}_${OPF}_task_graph.dot > ${APP}_${OPF}_task_graph.png
else
    echo "Not plotting the task graph!"
fi
echo "Done!"

