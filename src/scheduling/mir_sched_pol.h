#ifndef SCHED_POL_H
#define SCHED_POL_H 1

#include <stdint.h>
#include <stdlib.h>
#include <stdint.h>

#include "mir_task.h"
#include "mir_queue.h"
#include "mir_types.h"

BEGIN_C_DECLS

extern struct mir_sched_pol_t policy_central;
extern struct mir_sched_pol_t policy_ws;
#ifdef MIR_MEM_POL_ENABLE
extern size_t g_numa_schedule_footprint_config;
extern struct mir_sched_pol_t policy_numa;
#endif
extern struct mir_sched_pol_t policy_central_stack;
extern struct mir_sched_pol_t policy_ws_de;
extern struct mir_sched_pol_t policy_ws_de_node;

struct mir_sched_pol_t {
    // Data structures
    struct mir_queue_t** queues;
    struct mir_queue_t** alt_queues;
    uint16_t num_queues;
    uint32_t queue_capacity;
    const char* name;

    // Interfaces
    void (*config)(const char* conf_str);
    void (*create)();
    void (*destroy)();
    int (*push)(struct mir_worker_t*, struct mir_task_t*);
    int (*pop)(struct mir_task_t**);
};

struct mir_sched_pol_t* mir_sched_pol_get_by_name(const char* name);

END_C_DECLS

#endif
