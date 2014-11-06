#!/bin/bash

APP=bintree
OUTLINE_FUNCTIONS=ol_node_0,ol_node_1
CALLED_FUNCTIONS=fib,node
INPUT="12 20"
MIR_CONF="-w=1 -i -g -p"
OFILE_PREFIX="large"
PROCESS_PROFILE_DATA=1

# Profile and generate data
. ${MIR_ROOT}/programs/common/stub-profile.sh

