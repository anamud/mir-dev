#ifndef MIR_DATA_FOOTPRINT_H
#define MIR_DATA_FOOTPRINT_H

#include <stdint.h>
#include <stdlib.h>
#include "mir_mem_pol.h"

enum mir_data_access_t 
{
    MIR_DATA_ACCESS_READ = 0,
    MIR_DATA_ACCESS_WRITE,
    MIR_DATA_ACCESS_NUM_TYPES
};
typedef enum mir_data_access_t mir_data_access_t;

struct mir_data_footprint_t
{
    void* base;
    size_t type;
    uint64_t start;
    uint64_t end;
    uint64_t row_sz; // FIXME: This restricts footprints to square blocks
    mir_data_access_t data_access;
    void* part_of;
};

void mir_data_footprint_copy(struct mir_data_footprint_t* dest, struct mir_data_footprint_t* src);

// Note: This adds to dist
void mir_data_footprint_get_dist(struct mir_mem_node_dist_t* dist, struct mir_data_footprint_t* footprint);

#endif /* end of include guard: MIR_DATA_FOOTPRINT_H */

