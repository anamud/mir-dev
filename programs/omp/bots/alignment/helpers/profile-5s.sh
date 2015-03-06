#!/bin/bash
set -x

APP=alignment
OUTLINE_FUNCTIONS=pairalign._omp_fn.0,pairalign._omp_fn.1
CALLED_FUNCTIONS=del,add,calc_score,get_matrix,forward_pass,reverse_pass,diff,tracepath,pairalign,pairalign_seq,init_matrix,pairalign_init,align_init,align,align_seq_init,align_seq,align_end,align_verify,strlcpy,fill_chartab,encode,alloc_aln,get_seq,readseqs
INPUT="inputs/prot.200.aa"
MIR_CONF="-w=1 -i -g -p"
OFILE_PREFIX="5s"
PROCESS_PROFILE_DATA=1

# Profile and generate data
. ${MIR_ROOT}/programs/common/stub-profile.sh

