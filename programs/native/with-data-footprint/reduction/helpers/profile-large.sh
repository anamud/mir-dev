#!/bin/bash

APP=reduction
OUTLINE_FUNCTIONS=for_task_wrapper,reduce_wrapper
CALLED_FUNCTIONS=reduce,for_task
INPUT="12 512"
MIR_CONF="-w=1 -i -g -p"
OFILE_PREFIX="large"
PROCESS_PROFILE_DATA=1

. ${MIR_ROOT}/programs/common/stub-profile.sh

