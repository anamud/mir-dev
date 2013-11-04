#ifndef  MIR_TYPES_H
#define  MIR_TYPES_H

#include <stdint.h>
#include <stdlib.h>
#include "mir_defines.h"

// A generic id type
struct mir_id_t
{/*{{{*/
    uint64_t uid;
};/*}}}*/
typedef struct mir_id_t mir_id_t;

struct mir_sbuf_t 
{/*{{{*/
    int32_t buf[MIR_SBUF_SIZE];
    uint16_t size;
};/*}}}*/

#endif   
