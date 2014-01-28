#ifndef MIR_UTILS_H
#define MIR_UTILS_H 1

#include <stdlib.h>
#include "mir_types.h"

BEGIN_C_DECLS 

int mir_pstack_set_size(size_t sz);

int mir_get_num_threads();

int mir_get_threadid();

END_C_DECLS 

#endif
