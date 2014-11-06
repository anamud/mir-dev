#!/bin/bash

APP=map-mc
OUTLINE_FUNCTIONS=map_wrapper
CALLED_FUNCTIONS=map
INPUT="4 4"
MIR_CONF="-w=1 -i -g -p"
OFILE_PREFIX="test"
PROCESS_PROFILE_DATA=1

. ${MIR_ROOT}/programs/common/stub-profile.sh

