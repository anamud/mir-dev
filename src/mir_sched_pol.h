#ifndef SCHED_POL_H
#define SCHED_POL_H 1

#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>

#include "mir_task.h"
#include "mir_queue.h"

struct mir_sched_pol_t
{
    // Data structures
    struct mir_queue_t** queues;
    struct mir_queue_t** alt_queues;
    uint16_t num_queues;
    uint32_t queue_capacity;
    const char* name;

    // Interfaces
    void (*config) (const char* conf_str);
    void (*create) ();
    void (*destroy) ();
    void (*push) (struct mir_task_t* );
    bool (*pop) (struct mir_task_t**);
};

struct mir_sched_pol_t* mir_sched_pol_get_by_name(const char* name);

unsigned long mir_sched_pol_get_comm_cost(uint16_t node, struct mir_mem_node_dist_t* dist);

#endif 
