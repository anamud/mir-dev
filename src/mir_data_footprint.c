#include "mir_data_footprint.h"

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
}/*}}}*/
