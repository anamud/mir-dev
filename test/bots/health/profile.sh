#!/bin/bash

APP=health
TASKS=smp_ol_sim_village_par_0_unpacked
CALLED_FUNCS=addList,allocate_village,check_patients_assess_par,check_patients_inside,check_patients_population,check_patients_realloc,check_patients_waiting,check_village,my_rand,put_in_hosp,read_input_data,removeList
INPUT="inputs/test.input"
PLOT_TASK_GRAPH=0

. ${MIR_ROOT}/scripts/task-graph/stub-profile.sh
