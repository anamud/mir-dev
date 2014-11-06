#!/bin/bash

APP=floorplan
OUTLINE_FUNCTIONS=smp_ol_add_cell_0_unpacked
CALLED_FUNCTIONS=starts,lay_down,add_cell_ser,add_cell,memcpy
INPUT="./inputs/input.15 7"
MIR_CONF="-w=1 -i -g -l=128 -p"
OFILE_PREFIX="5s"
PROCESS_PROFILE_DATA=1

# Profile and generate data
. ${MIR_ROOT}/programs/common/stub-profile.sh

