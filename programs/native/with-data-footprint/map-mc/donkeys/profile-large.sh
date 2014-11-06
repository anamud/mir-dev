#!/bin/bash

APP=map-mc
OUTLINE_FUNCTIONS=map_wrapper
CALLED_FUNCTIONS=map
INPUT="384 128"
MIR_CONF="-w=1 -i -g -p"
OFILE_PREFIX="large"
PROCESS_PROFILE_DATA=1

. ${MIR_ROOT}/programs/common/stub-profile.sh

