#include "mir_runtime.h"
#include "scheduling/mir_sched_pol.h"
#include "mir_worker.h"
#include "mir_task.h"
#include "mir_queue.h"
#include "mir_recorder.h"
#include "mir_memory.h"
#include "mir_utils.h"
#include "mir_defines.h"
#include "mir_mem_pol.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#ifdef MIR_MEM_POL_ENABLE
size_t g_numa_schedule_footprint_config = 0;

void create_numa()
{ /*{{{*/
    struct mir_sched_pol_t* sp = runtime->sched_pol;
    MIR_ASSERT(NULL != sp);

    // Create node private task queues
    sp->num_queues = runtime->arch->num_nodes;
    sp->queues = mir_malloc_int(sp->num_queues * sizeof(struct mir_queue_t*));
    MIR_CHECK_MEM(NULL != sp->queues);

    for (int i = 0; i < sp->num_queues; i++) {
        sp->queues[i] = mir_queue_create(sp->queue_capacity);
        MIR_ASSERT(NULL != sp->queues[i]);
    }

    // Create node private alternate task queues
    sp->alt_queues = mir_malloc_int(sp->num_queues * sizeof(struct mir_queue_t*));
    MIR_CHECK_MEM(NULL != sp->alt_queues);

    for (int i = 0; i < sp->num_queues; i++) {
        sp->alt_queues[i] = mir_queue_create(sp->queue_capacity);
        MIR_ASSERT(NULL != sp->alt_queues[i]);
    }
} /*}}}*/

void destroy_numa()
{ /*{{{*/
    struct mir_sched_pol_t* sp = runtime->sched_pol;
    MIR_ASSERT(NULL != sp);

    // Free queues
    for (int i = 0; i < sp->num_queues; i++) {
        MIR_ASSERT(NULL != sp->queues[i]);
        mir_queue_destroy(sp->queues[i]);
        sp->queues[i] = NULL;
    }

    MIR_ASSERT(NULL != sp->queues);
    mir_free_int(sp->queues, sizeof(struct mir_queue_t*) * sp->num_queues);
    sp->queues = NULL;

    // Free alt_queues
    for (int i = 0; i < sp->num_queues; i++) {
        MIR_ASSERT(NULL != sp->alt_queues[i]);
        mir_queue_destroy(sp->alt_queues[i]);
        sp->alt_queues[i] = NULL;
    }

    MIR_ASSERT(NULL != sp->alt_queues);
    mir_free_int(sp->alt_queues, sizeof(struct mir_queue_t*) * sp->num_queues);
    sp->alt_queues = NULL;
} /*}}}*/

static inline int is_data_dist_significant(struct mir_mem_node_dist_t* dist)
{ /*{{{*/
    // Lower limit by default is the L3 cache available per core
    size_t low_limit = (runtime->arch->llc_size_KB * 1024) / (runtime->arch->num_cores / runtime->arch->num_nodes);
    if (g_numa_schedule_footprint_config > 0)
        low_limit = g_numa_schedule_footprint_config;

    // Get dist stats
    struct mir_mem_node_dist_stat_t stat;
    mir_mem_node_dist_get_stat(&stat, dist);
    //MIR_DEBUG("Dist stats: %lu %lu %lu %.3f %.3f [%lu, 0.0].", stat.sum, stat.min, stat.max, stat.mean, stat.sd, low_limit);

    // Check if data from all node is large enough
    if (stat.sum < low_limit)
        return 0;

    // FIXME: Refine this!
    if (stat.sd == 0.0)
        return 0;

    return 1;
} /*}}}*/

int push_numa(struct mir_worker_t* this_worker, struct mir_task_t* task)
{ /*{{{*/
    MIR_ASSERT(NULL != task);

    struct mir_worker_t* least_cost_worker = NULL;
    MIR_ASSERT(NULL != this_worker);
    int push_to_alt_queue = 0;
    int pushed = 1;

    // Push task onto node with the least access cost to read data footprint
    // If no data footprint, push task onto this worker's node
    struct mir_mem_node_dist_t* dist = mir_task_get_mem_node_dist(task, MIR_DATA_ACCESS_READ);
    //struct mir_mem_node_dist_t* dist = mir_task_get_mem_node_dist(task, MIR_DATA_ACCESS_WRITE);
    if (dist) {
        if (is_data_dist_significant(dist) == 0) {
            /*MIR_LOG_INFO("Task %" MIR_FORMSPEC_UL " ignored.", task->id.uid);*/
            least_cost_worker = this_worker;
            push_to_alt_queue = 1;
            if (runtime->enable_worker_stats == 1)
                task->comm_cost = mir_mem_node_dist_get_comm_cost(dist, runtime->arch->node_of(least_cost_worker->cpu_id));
        }
        else {
            uint16_t prev_node = runtime->arch->num_nodes + 1;
            unsigned long least_comm_cost = -1;
            int bias = this_worker->bias;
            int i = bias;
            do {
                struct mir_worker_t* worker = &runtime->workers[i];
                MIR_ASSERT(NULL != worker);
                uint16_t node = runtime->arch->node_of(worker->cpu_id);
                if (node != prev_node) {
                    prev_node = node;
                    unsigned long comm_cost = mir_mem_node_dist_get_comm_cost(dist, node);
                    if (comm_cost < least_comm_cost) {
                        least_comm_cost = comm_cost;
                        least_cost_worker = worker;
                    }
                }
                i = (i + 1) % runtime->num_workers;
            } while (i != bias);
            mir_worker_update_bias(this_worker);

            if (runtime->enable_worker_stats == 1)
                task->comm_cost = least_comm_cost;

            /*MIR_LOG_INFO("Task %" MIR_FORMSPEC_UL " scheduled on node %d.", task->id.uid, runtime->arch->node_of(least_cost_worker->cpu_id));*/
        }
    }
    else {
        least_cost_worker = this_worker;
        push_to_alt_queue = 1;
    }

    // Push task to worker's queue
    struct mir_queue_t* queue = NULL;
    if (push_to_alt_queue == 1)
        queue = runtime->sched_pol->alt_queues[runtime->arch->node_of(least_cost_worker->cpu_id)];
    else
        queue = runtime->sched_pol->queues[runtime->arch->node_of(least_cost_worker->cpu_id)];
    MIR_ASSERT(NULL != queue);
    if (0 == mir_queue_push(queue, (void*)task)) {
#ifdef MIR_INLINE_TASK_IF_QUEUE_FULL
        pushed = 0;
        mir_task_execute(task);
        // Update stats
        if (runtime->enable_worker_stats == 1)
            this_worker->statistics->num_tasks_inlined++;
#else
        MIR_LOG_ERR("Cannot enque task. Increase queue capacity using MIR_CONF.");
#endif
    }
    else {
        //MIR_LOG_INFO("Task scheduled on worker %d.", least_cost_worker->id);
        __sync_fetch_and_add(&g_num_tasks_waiting, 1);

        // Update stats
        if (runtime->enable_worker_stats == 1)
            this_worker->statistics->num_tasks_created++;
    }

    //MIR_RECORDER_STATE_END(NULL, 0);

    return pushed;
} /*}}}*/

int pop_numa(struct mir_task_t** task)
{ /*{{{*/
    int found = 0;
    struct mir_sched_pol_t* sp = runtime->sched_pol;
    MIR_ASSERT(NULL != sp);
    uint32_t num_queues = sp->num_queues;
    struct mir_worker_t* worker = mir_worker_get_context();
    MIR_ASSERT(NULL != worker);
    uint16_t node = runtime->arch->node_of(worker->cpu_id);

    // Pop from own node alt queue
    //MIR_RECORDER_STATE_BEGIN(MIR_STATE_TPOP);

    struct mir_queue_t* queue = sp->alt_queues[node];
    MIR_ASSERT(NULL != queue);
    if (mir_queue_size(queue) > 0) {
        *task = NULL;
        mir_queue_pop(queue, (void**)&(*task));
        if (*task) {
            // Update stats
            if (runtime->enable_worker_stats == 1) {
                // task->comm-cost already calculated in push
                if ((*task)->comm_cost != -1) {
                    mir_worker_statistics_update_comm_cost(worker->statistics, (*task)->comm_cost);
                }

                worker->statistics->num_tasks_owned++;
            }

            __sync_fetch_and_sub(&g_num_tasks_waiting, 1);
            T_DBG("Dq", *task);

            found = 1;
        }
    }

    //MIR_RECORDER_STATE_END(NULL, 0);

    if (found)
        return found;

    // Pop from own node queue
    //MIR_RECORDER_STATE_BEGIN(MIR_STATE_TPOP);

    queue = sp->queues[node];
    MIR_ASSERT(NULL != queue);
    if (mir_queue_size(queue) > 0) {
        *task = NULL;
        mir_queue_pop(queue, (void**)&(*task));
        if (*task) {
            // Update stats
            if (runtime->enable_worker_stats == 1) {
                // task->comm-cost already calculated in push
                if ((*task)->comm_cost != -1) {
                    mir_worker_statistics_update_comm_cost(worker->statistics, (*task)->comm_cost);
                }

                worker->statistics->num_tasks_owned++;
            }

            __sync_fetch_and_sub(&g_num_tasks_waiting, 1);
            T_DBG("Dq", *task);

            found = 1;
        }
    }

    //MIR_RECORDER_STATE_END(NULL, 0);

    if (found)
        return found;

// Next try to pop from other queues
// First check in alt queues

#if 1
    //MIR_RECORDER_STATE_BEGIN(MIR_STATE_TSTEAL);
    uint16_t ctr = node + 1;
    if (ctr == num_queues)
        ctr = 0;

    while (ctr != node) {
        struct mir_queue_t* queue = sp->alt_queues[ctr];
        MIR_ASSERT(NULL != queue);
        if (mir_queue_size(queue) > 0) {
            *task = NULL;
            mir_queue_pop(queue, (void**)&(*task));
            if (*task) {
                // Update stats
                if (runtime->enable_worker_stats == 1) {
                    struct mir_mem_node_dist_t* dist = mir_task_get_mem_node_dist(*task, MIR_DATA_ACCESS_READ);
                    if (dist) {
                        (*task)->comm_cost = mir_mem_node_dist_get_comm_cost(dist, node);
                        mir_worker_statistics_update_comm_cost(worker->statistics, (*task)->comm_cost);
                    }

                    worker->statistics->num_tasks_stolen++;
                }

                __sync_fetch_and_sub(&g_num_tasks_waiting, 1);
                T_DBG("St", *task);

                found = 1;
                break;
            }
        }

        // Incremenet counter and wrap around
        ctr++;
        if (ctr == num_queues)
            ctr = 0;
    }

//MIR_RECORDER_STATE_END(NULL, 0);
#endif

    if (found)
        return found;

// Now check in other queues
// Nearest queues get searched first
// Pop by hop count

#if 1
    //MIR_RECORDER_STATE_BEGIN(MIR_STATE_TSTEAL);
    uint16_t neighbors[runtime->arch->num_nodes];
    uint16_t count;

    for (int d = 1; d <= runtime->arch->diameter && found != 1; d++) { /*{{{*/
        count = runtime->arch->vicinity_of(neighbors, node, d);
        for (int i = 0; i < count; i++) {
            // FIXME: Decision bias possible here
            struct mir_queue_t* queue = sp->queues[neighbors[i]];
            MIR_ASSERT(NULL != queue);
            uint32_t queue_sz = mir_queue_size(queue);
            size_t low_limit = (runtime->arch->num_cores / runtime->arch->num_nodes) * d;
            //size_t low_limit = (runtime->arch->num_cores/runtime->arch->num_nodes);
            //size_t low_limit = 0;
            if (queue_sz > low_limit) {
                *task = NULL;
                mir_queue_pop(queue, (void**)&(*task));
                if (*task) {
                    // Update stats
                    if (runtime->enable_worker_stats == 1) {
                        struct mir_mem_node_dist_t* dist = mir_task_get_mem_node_dist(*task, MIR_DATA_ACCESS_READ);
                        //struct mir_mem_node_dist_t* dist = mir_task_get_mem_node_dist(*task, MIR_DATA_ACCESS_WRITE);
                        if (dist) {
                            (*task)->comm_cost = mir_mem_node_dist_get_comm_cost(dist, node);
                            mir_worker_statistics_update_comm_cost(worker->statistics, (*task)->comm_cost);
                            worker->statistics->num_comm_tasks_stolen_by_diameter[d - 1]++;
                        }

                        worker->statistics->num_tasks_stolen++;
                    }

                    __sync_fetch_and_sub(&g_num_tasks_waiting, 1);
                    T_DBG("Dq", *task);

                    found = 1;
                    break;
                }
            }
        }
    } /*}}}*/

//MIR_RECORDER_STATE_END(NULL, 0);
#endif

    return found;
} /*}}}*/

struct mir_sched_pol_t policy_numa = { /*{{{*/
    .num_queues = MIR_WORKER_MAX_COUNT,
    .queue_capacity = MIR_QUEUE_MAX_CAPACITY,
    .queues = NULL,
    .alt_queues = NULL,
    .name = "numa",
    .create = create_numa,
    .destroy = destroy_numa,
    .push = push_numa,
    .pop = pop_numa
}; /*}}}*/
#endif

