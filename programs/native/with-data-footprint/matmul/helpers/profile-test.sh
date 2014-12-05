#!/bin/bash

APP=matmul
OUTLINE_FUNCTIONS=compute_task_wrapper
CALLED_FUNCTIONS=matmul,compute_task
INPUT="32"
MIR_CONF="-w=1 -g -p"
OFILE_PREFIX="test"
PROCESS_PROFILE_DATA=1

. ${MIR_ROOT}/programs/common/stub-profile.sh

