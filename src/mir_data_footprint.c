#include "mir_data_footprint.h"
#include "mir_mem_pol.h"
#include "mir_debug.h"

void mir_data_footprint_copy(struct mir_data_footprint_t* dest, struct mir_data_footprint_t* src)
{/*{{{*/
    if(!dest || !src)
        return;

    // Copy elements
    dest->base = src->base;
    dest->type = src->type;
    dest->start = src->start;
    dest->end = src->end;
    dest->row_sz = src->row_sz;
    dest->data_access = src->data_access;
    dest->part_of = src->part_of;
}/*}}}*/

void mir_data_footprint_get_dist(struct mir_mem_node_dist_t* dist, struct mir_data_footprint_t* footprint)
{/*{{{*/
    uint64_t row_sz = footprint->row_sz;
    if(row_sz > 1)
    {
        for(uint64_t brow=0; brow<=footprint->end; brow++)
        {
            void* base = footprint->base + brow*row_sz*footprint->type;
            mir_mem_get_dist( dist, base, 
                    (footprint->end - footprint->start + 1) * footprint->type, 
                    footprint->part_of );

            /*MIR_INFORM("Footprint composed of these addresses:\n");*/
            /*MIR_INFORM("%p[%lu-%lu]\n", base, footprint->start, footprint->end);*/
        }
    }
    else
    {
        mir_mem_get_dist( dist, footprint->base, 
                (footprint->end - footprint->start + 1) * footprint->type, 
                footprint->part_of );
        /*MIR_INFORM("Footprint composed of these addresses:\n");*/
        /*MIR_INFORM("%p[%lu-%lu]\n", footprint->base, footprint->start, footprint->end);*/
    }
}/*}}}*/
