#include "mir_data_footprint.h"
#include "mir_utils.h"

#ifdef MIR_MEM_POL_ENABLE
// FIXME: Multi-tasking function
void mir_data_footprint_get_dist(struct mir_mem_node_dist_t* dist, const struct mir_data_footprint_t* footprint)
{/*{{{*/
    // Check
    MIR_ASSERT(dist != NULL);
    MIR_ASSERT(footprint != NULL);

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
#endif
