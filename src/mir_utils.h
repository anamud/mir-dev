#ifndef MIR_UTILS_H
#define MIR_UTILS_H 1

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <assert.h>
#include "mir_types.h"

#define MIR_ASSERT(cond) assert(cond)

#ifdef MIR_ENABLE_DEBUG
#define MIR_DEBUG(...) do{ fprintf( stderr, __VA_ARGS__ ); } while(0)
#else
#define MIR_DEBUG(...) do{ } while (0)
#endif

#define MIR_INFORM(...) do{ fprintf( stderr, __VA_ARGS__ ); } while(0)

#define MIR_ABORT(...) do{ fprintf( stderr, __VA_ARGS__ ); exit(1); } while(0)
BEGIN_C_DECLS 

int mir_pstack_set_size(size_t sz);

/*LIBINT*/ int mir_get_num_threads();

/*LIBINT*/ int mir_get_threadid();

/*LIBINT*/ void mir_sleep_ms(uint32_t msec);

/*LIBINT*/ void mir_sleep_us(uint32_t usec);

/*LIBINT*/ uint64_t mir_get_cycles();

END_C_DECLS 

#endif
