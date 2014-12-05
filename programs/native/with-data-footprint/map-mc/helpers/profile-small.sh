#!/bin/bash

APP=map-mc
OUTLINE_FUNCTIONS=map_wrapper
CALLED_FUNCTIONS=map
INPUT="4 10"
MIR_CONF="-w=1 -i -g -p"
OFILE_PREFIX="small"
PROCESS_PROFILE_DATA=1

. ${MIR_ROOT}/programs/common/stub-profile.sh

