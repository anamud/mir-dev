#ifndef  MIR_TYPES_H
#define  MIR_TYPES_H

#include <stdint.h>
#include <inttypes.h>
#include <stdlib.h>
#include "mir_defines.h"

#ifdef __tile__
#define MIR_FORMSPEC_UL "llu"
#define MIR_FORMSPEC_SL "lld"
#else
#define MIR_FORMSPEC_UL PRIu64
#define MIR_FORMSPEC_SL PRId64
#endif

// Interop with C++
// Do not mangle
#undef BEGIN_C_DECLS
#undef END_C_DECLS
#ifdef __cplusplus
# define BEGIN_C_DECLS extern "C" {
# define END_C_DECLS }
#else
# define BEGIN_C_DECLS /* empty */
# define END_C_DECLS /* empty */
#endif

BEGIN_C_DECLS 

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

struct mir_time_list_t
{/*{{{*/
    uint64_t time;
    struct mir_time_list_t* next;
};/*}}}*/

END_C_DECLS 

#endif   
