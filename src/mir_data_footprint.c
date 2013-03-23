#include "mir_data_footprint.h"
#include "mir_mem_pol.h"

void mir_data_footprint_copy(struct mir_data_footprint_t* dest, struct mir_data_footprint_t* src)
{/*{{{*/
    if(!dest || !src)
        return;

    // Copy elements
    dest->base = src->base;
    dest->type = src->type;
    dest->start = src->start;
    dest->end = src->end;
    dest->data_access = src->data_access;
    dest->part_of = src->part_of;
}/*}}}*/

struct mir_mem_node_dist_t* mir_data_footprint_get_dist(struct mir_data_footprint_t* footprint)
{
    return mir_mem_get_dist( footprint->base, 
            (footprint->end - footprint->start + 1) * footprint->type, 
            footprint->part_of );
}
