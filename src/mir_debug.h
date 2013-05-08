#ifndef  MIR_DEBUG_H
#define  MIR_DEBUG_H 1

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <assert.h>

#define MIR_ASSERT(cond) assert(cond)

#ifdef MIR_ENABLE_DEBUG
#define MIR_DEBUG(...) do{ fprintf( stderr, __VA_ARGS__ ); } while(0)
#else
#define MIR_DEBUG(...) do{ } while (0)
#endif

#define MIR_INFORM(...) do{ fprintf( stderr, __VA_ARGS__ ); } while(0)

#define MIR_ABORT(...) do{ fprintf( stderr, __VA_ARGS__ ); exit(1); } while(0)

void mir_sleep_ms(uint32_t msec);

void mir_sleep_us(uint32_t usec);
#endif
