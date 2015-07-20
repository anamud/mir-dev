#include "mir_runtime.h"
#include "scheduling/mir_sched_pol.h"
#include "mir_worker.h"
#include "mir_task.h"
#include "mir_queue.h"
#include "mir_dequeue.h"
#include "mir_recorder.h"
#include "mir_memory.h"
#include "mir_utils.h"
#include "mir_defines.h"
#ifdef MIR_MEM_POL_ENABLE
#include "mir_mem_pol.h"
#endif

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

extern uint32_t g_num_tasks_waiting;
extern struct mir_runtime_t* runtime;

void create_ws_de_node()
{ /*{{{*/
    struct mir_sched_pol_t* sp = runtime->sched_pol;
    MIR_ASSERT(NULL != sp);

    // Create worker private task queues
    sp->num_queues = runtime->num_workers;
    sp->queues = (struct mir_queue_t**)mir_malloc_int(sp->num_queues * sizeof(mir_dequeue_t*));
    MIR_CHECK_MEM(NULL != sp->queues);

    for (int i = 0; i < sp->num_queues; i++) {
        sp->queues[i] = (struct mir_queue_t*)newWSDeque(sp->queue_capacity);
        MIR_ASSERT(NULL != sp->queues[i]);
    }
} /*}}}*/

void destroy_ws_de_node()
{           /*{{{*/
    return; // FIXME: Bad exit. Workers bang on queues which are freed.
    struct mir_sched_pol_t* sp = runtime->sched_pol;
    MIR_ASSERT(NULL != sp);

    // Free queues
    for (int i = 0; i < sp->num_queues; i++) {
        MIR_ASSERT(NULL != sp->queues[i]);
        freeWSDeque((mir_dequeue_t*)sp->queues[i]);
        sp->queues[i] = NULL;
    }

    MIR_ASSERT(NULL != sp->queues);
    mir_free_int(sp->queues, sizeof(mir_dequeue_t*) * sp->num_queues);
    sp->queues = NULL;
} /*}}}*/

int push_ws_de_node(struct mir_worker_t* worker, struct mir_task_t* task)
{ /*{{{*/
    MIR_ASSERT(NULL != task);
    MIR_ASSERT(NULL != worker);

    int pushed = 1;

    // ws has per-worker queues
    mir_dequeue_t* queue = (mir_dequeue_t*)runtime->sched_pol->queues[worker->id];
    MIR_ASSERT(NULL != queue);
    if (rtsFalse == pushWSDeque(queue, (void*)task)) {
#ifdef MIR_INLINE_TASK_IF_QUEUE_FULL
        pushed = 0;
        mir_task_execute(task);
        // Update stats
        if (runtime->enable_worker_stats == 1)
            worker->statistics->num_tasks_inlined++;
#else
        MIR_LOG_ERR("Cannot enqueue task. Increase queue capacity using MIR_CONF.");
#endif
    }
    else {
        __sync_fetch_and_add(&g_num_tasks_waiting, 1);
        // Update stats
        if (runtime->enable_worker_stats == 1)
            worker->statistics->num_tasks_created++;
    }

    //MIR_RECORDER_STATE_END(NULL, 0);

    return pushed;
} /*}}}*/

int pop_ws_de_node(struct mir_task_t** task)
{ /*{{{*/
    int found = 0;
    struct mir_sched_pol_t* sp = runtime->sched_pol;
    MIR_ASSERT(NULL != sp);
    uint32_t num_queues = sp->num_queues;
    struct mir_worker_t* worker = mir_worker_get_context();
    MIR_ASSERT(NULL != worker);
    uint16_t node = runtime->arch->node_of(worker->cpu_id);

    // First try to pop from own queue
    //MIR_RECORDER_STATE_BEGIN(MIR_STATE_TPOP);

    mir_dequeue_t* queue = (mir_dequeue_t*)sp->queues[worker->id];
    if (looksEmptyWSDeque(queue) == rtsFalse) {
        *task = NULL;
        *task = (struct mir_task_t*)popWSDeque(queue);
        if (*task) {
            bool grab = __sync_bool_compare_and_swap(&((*task)->taken), 0, 1);
            if (grab == true) {
                // Update stats
                if (runtime->enable_worker_stats == 1) {
#ifdef MIR_MEM_POL_ENABLE
                    struct mir_mem_node_dist_t* dist = mir_task_get_mem_node_dist(*task, MIR_DATA_ACCESS_READ);
                    if (dist) {
                        (*task)->comm_cost = mir_mem_node_dist_get_comm_cost(dist, node);
                        mir_worker_statistics_update_comm_cost(worker->statistics, (*task)->comm_cost);
                    }
#endif

                    worker->statistics->num_tasks_owned++;
                }

                __sync_fetch_and_sub(&g_num_tasks_waiting, 1);
                MIR_ASSERT(g_num_tasks_waiting >= 0);
                T_DBG("Dq", *task);

                found = 1;
            }
        }
    }

    //MIR_RECORDER_STATE_END(NULL, 0);

    if (found)
        return found;

    // Next try to pop from other queues within own node

    uint16_t ctr = worker->id + 1;
    if (ctr == num_queues)
        ctr = 0;

    while (ctr != worker->id) {
        uint16_t victim_node = runtime->arch->node_of(ctr);
        if (node != victim_node) {
            // Incremenet counter and wrap around
            ctr++;
            if (ctr == num_queues)
                ctr = 0;
            continue;
        }

        mir_dequeue_t* queue = (mir_dequeue_t*)sp->queues[ctr];
        if (looksEmptyWSDeque(queue) == rtsFalse) {
            *task = NULL;
            *task = (struct mir_task_t*)stealWSDeque(queue);
            if (*task) {
                bool grab = __sync_bool_compare_and_swap(&((*task)->taken), 0, 1);
                if (grab == true) {
                    // Update stats
                    if (runtime->enable_worker_stats == 1) {
#ifdef MIR_MEM_POL_ENABLE
                        struct mir_mem_node_dist_t* dist = mir_task_get_mem_node_dist(*task, MIR_DATA_ACCESS_READ);
                        if (dist) {
                            (*task)->comm_cost = mir_mem_node_dist_get_comm_cost(dist, node);
                            mir_worker_statistics_update_comm_cost(worker->statistics, (*task)->comm_cost);
                        }
#endif

                        worker->statistics->num_tasks_stolen++;
                    }

                    __sync_fetch_and_sub(&g_num_tasks_waiting, 1);
                    MIR_ASSERT(g_num_tasks_waiting >= 0);
                    T_DBG("St", *task);

                    found = 1;
                    break;
                }
            }
        }

        // Incremenet counter and wrap around
        ctr++;
        if (ctr == num_queues)
            ctr = 0;
    }

    if (found)
        return found;

    // Next try to pop from other queues within other nodes
    for (int d = 1; d <= runtime->arch->diameter && found != 1; d++) { /*{{{*/
        uint16_t neighbors[runtime->arch->num_nodes];
        uint16_t count = runtime->arch->vicinity_of(neighbors, node, d);
        for (int i = 0; i < count && found != 1; i++) {
            struct mir_sbuf_t cpus;
            runtime->arch->cpus_of(&cpus, neighbors[i]);
            for (int j = 0; j < cpus.size; j++) {
                mir_dequeue_t* queue = (mir_dequeue_t*)sp->queues[cpus.buf[j]];
                if (looksEmptyWSDeque(queue) == rtsFalse) {
                    *task = NULL;
                    *task = (struct mir_task_t*)stealWSDeque(queue);
                    if (*task) {
                        bool grab = __sync_bool_compare_and_swap(&((*task)->taken), 0, 1);
                        if (grab == true) {
                            // Update stats
                            if (runtime->enable_worker_stats == 1) {
                                /*#ifdef MIR_MEM_POL_ENABLE*/ /*{{{*/
                                /*struct mir_mem_node_dist_t* dist = mir_task_get_mem_node_dist(*task, MIR_DATA_ACCESS_READ);*/
                                /*if(dist)*/
                                /*{*/
                                /*(*task)->comm_cost = mir_mem_node_dist_get_comm_cost(dist, node);*/
                                /*mir_worker_statistics_update_comm_cost(worker->statistics, (*task)->comm_cost);*/
                                /*}*/
                                /*#endif*/ /*}}}*/

                                worker->statistics->num_tasks_stolen++;
                            }

                            __sync_fetch_and_sub(&g_num_tasks_waiting, 1);
                            MIR_ASSERT(g_num_tasks_waiting >= 0);
                            T_DBG("St", *task);

                            found = 1;
                            break;
                        }
                    }
                }
            }
        }
    } /*}}}*/

    return found;
} /*}}}*/

struct mir_sched_pol_t policy_ws_de_node = { /*{{{*/
    .num_queues = MIR_WORKER_MAX_COUNT,
    .queue_capacity = MIR_QUEUE_MAX_CAPACITY,
    .queues = NULL,
    .alt_queues = NULL,
    .name = "ws-de-node",
    .create = create_ws_de_node,
    .destroy = destroy_ws_de_node,
    .push = push_ws_de_node,
    .pop = pop_ws_de_node
}; /*}}}*/

