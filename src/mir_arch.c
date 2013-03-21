#include "mir_arch.h"
#include "mir_debug.h"
#include "mir_defines.h"
#include "mir_types.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern struct mir_arch_t arch_adk;
extern struct mir_arch_t arch_gatti;
extern struct mir_arch_t arch_gothmog;
extern struct mir_arch_t arch_tilepro64;

// WARNING and NOTE: Make sure predef architecture count == num entries in predef architecture struct
#define MIR_ARCH_NUM_PREDEF 4
static struct mir_arch_t* mir_arch_predef[MIR_ARCH_NUM_PREDEF] = 
{/*{{{*/
    &arch_adk,
    &arch_gatti,
    &arch_gothmog,
    &arch_tilepro64
};/*}}}*/

struct mir_arch_t* mir_arch_create_by_query()
{/*{{{*/
    struct mir_arch_t* arch = NULL;

#ifdef __tile__
    arch = &arch_tilepro64;
#else
    // Run uname -n to get node name
    FILE* fpipe;
    fpipe = popen("uname -n", "r");
    if(!fpipe)
        MIR_ABORT(MIR_ERROR_STR "Cannot open pipe!\n");

    // Get node name into buffer and strip it of the newline at end
    char arch_name[MIR_SHORT_NAME_LEN];
    fgets(arch_name, MIR_SHORT_NAME_LEN, fpipe);
    for(int i=0; i<MIR_SHORT_NAME_LEN; i++)
        if(arch_name[i] == '\n')
            arch_name[i] = '\0';

    // Compare node name with known architectures 
    // and set architecture
    for(int i=0; i<MIR_ARCH_NUM_PREDEF; i++)
    {
        if(0 == strcmp(arch_name, mir_arch_predef[i]->name))
            arch = mir_arch_predef[i];
    }
#endif

    return arch;
}/*}}}*/

