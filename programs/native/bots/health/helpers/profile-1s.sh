#!/bin/bash

APP=health
OUTLINE_FUNCTIONS=smp_ol_sim_village_par_0_unpacked
CALLED_FUNCTIONS=addList,allocate_village,check_patients_assess_par,check_patients_inside,check_patients_population,check_patients_realloc,check_patients_waiting,check_village,my_rand,put_in_hosp,read_input_data,removeList,sim_village_par
INPUT="inputs/prof-5s.input"
MIR_CONF="-w=1 -i -g -p"
OFILE_PREFIX="5s"
PROCESS_PROFILE_DATA=1

# Profile and generate data
. ${MIR_ROOT}/programs/common/stub-profile.sh

