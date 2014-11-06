#!/bin/bash

APP=map
OUTLINE_FUNCTIONS=map_wrapper,for_task_wrapper
CALLED_FUNCTIONS=map,for_task
INPUT="192 128"
MIR_CONF="-w=1 -i -g -p"
OFILE_PREFIX="medium"
PROCESS_PROFILE_DATA=1

. ${MIR_ROOT}/programs/common/stub-profile.sh

