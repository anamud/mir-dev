#include "mir_runtime.h"
#include "mir_sched_pol.h"
#include "mir_worker.h"
#include "mir_task.h"
#include "mir_queue.h"
#include "mir_recorder.h"
#include "mir_memory.h"
#include "mir_debug.h"
#include "mir_defines.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

extern uint32_t g_num_tasks_waiting;
extern struct mir_runtime_t* runtime;

static size_t schedule_cutoff_config = 0;

void config_numa (const char* conf_str)
{/*{{{*/
    char str[MIR_LONG_NAME_LEN];
    strcpy(str, conf_str);

    struct mir_sched_pol_t* sp = runtime->sched_pol;

    char* tok = strtok(str, " ");
    while(tok)
    {
        if(tok[0] == '-')
        {
            char c = tok[1];
            switch(c)
            {/*{{{*/
                case 'q':
                    if(tok[2] == '=')
                    {
                        char* s = tok+3;
                        sp->queue_capacity = atoi(s);
                        //MIR_INFORM(MIR_INFORM_STR "Setting queue capacity to %d\n", sp->queue_capacity);
                    }
                    else
                    {
                        MIR_ABORT(MIR_ERROR_STR "Incorrect MIR_CONF parameter [%c]\n", c);
                    }
                    break;
                case 'y':
                    if(tok[2] == '=')
                    {
                        char* s = tok+3;
                        schedule_cutoff_config = atoi(s);
                    }
                    else
                    {
                        MIR_ABORT(MIR_ERROR_STR "Incorrect MIR_CONF parameter [%c]\n", c);
                    }
                    break;
                default:
                    break;
            }/*}}}*/
        }
        tok = strtok(NULL, " ");
    }
}/*}}}*/

void create_numa ()
{/*{{{*/
    struct mir_sched_pol_t* sp = runtime->sched_pol;

    // Create node private task queues
    sp->num_queues = runtime->arch->num_nodes;
    sp->queues = (struct mir_queue_t**) mir_malloc_int (sp->num_queues * sizeof(struct mir_queue_t*));
    if(NULL == sp->queues)
        MIR_ABORT(MIR_ERROR_STR "Unable to create task queues!\n");

    for(int i=0; i< sp->num_queues; i++)
        sp->queues[i] = mir_queue_create(sp->queue_capacity);
}/*}}}*/

void destroy_numa ()
{/*{{{*/
    struct mir_sched_pol_t* sp = runtime->sched_pol;
    
    // Free queues
    for(int i=0; i<sp->num_queues ; i++)
        mir_queue_destroy(sp->queues[i]);

    mir_free_int(sp->queues, sizeof(struct mir_queue_t*) * sp->num_queues);
}/*}}}*/

static inline unsigned long get_comm_cost(uint16_t node, struct mir_mem_node_dist_t* dist)
{/*{{{*/
    unsigned long comm_cost = 0;

    for(int i=0; i<runtime->arch->num_nodes; i++)
        comm_cost += (dist->buf[i] * runtime->arch->comm_cost_of(node, i));

    return comm_cost;
}/*}}}*/

void push_numa (struct mir_task_t* task)
{/*{{{*/
    MIR_RECORDER_STATE_BEGIN(MIR_STATE_TSUBMIT);

    struct mir_worker_t* least_cost_worker = NULL;
    struct mir_worker_t* this_worker = mir_worker_get_context();

    // Push task onto node with the least access cost to read data footprint
    // If no data footprint, push task onto this worker's node
    struct mir_mem_node_dist_t* dist = mir_task_get_footprint_dist(task, MIR_DATA_ACCESS_READ);
    if(dist)
    {
        size_t limit = (runtime->arch->llc_size_KB * 1024) / (runtime->arch->num_cores / runtime->arch->num_nodes);

        if(schedule_cutoff_config > 0)
            limit = schedule_cutoff_config;
            
        if(mir_mem_node_dist_sum(dist) < limit)
        {
            least_cost_worker = this_worker;
            task->comm_cost = get_comm_cost(runtime->arch->node_of(least_cost_worker->id), dist);
        }
        else
        {
            uint16_t prev_node = runtime->arch->num_nodes + 1;
            unsigned long least_comm_cost = -1;
            for(int i=0; i<runtime->num_workers; i++)
            {
                struct mir_worker_t* worker = &runtime->workers[i];
                uint16_t node = runtime->arch->node_of(worker->id);
                if(node != prev_node)
                {
                    prev_node = node;
                    unsigned long comm_cost = get_comm_cost(node, dist);
                    if(comm_cost < least_comm_cost)
                    {
                        least_comm_cost = comm_cost;
                        least_cost_worker = worker;
                    }
                }
            }
            task->comm_cost = least_comm_cost;
        }
    }
    else
    {
        least_cost_worker = this_worker;
    }

    // Push task to worker's queue
    struct mir_queue_t* queue = runtime->sched_pol->queues[runtime->arch->node_of(least_cost_worker->id)];
    if( false == mir_queue_push(queue, (void*) task) )
    {
#ifdef MIR_SCHED_POL_INLINE_TASKS
        mir_task_execute(task);
        // Update stats
        this_worker->status->num_tasks_inlined++;
#else
        MIR_ABORT(MIR_ERROR_STR "Cannot enque task. Increase queue capacity using MIR_CONF.\n");
#endif
    }
    else
    {
        // Update stats
        __sync_fetch_and_add(&g_num_tasks_waiting, 1);
        least_cost_worker->status->num_tasks_spawned++;
    }

    MIR_RECORDER_STATE_END(NULL, 0);
}/*}}}*/

bool pop_numa (struct mir_task_t** task)
{/*{{{*/
    bool found = 0;
    struct mir_sched_pol_t* sp = runtime->sched_pol;
    uint32_t num_queues = sp->num_queues;
    struct mir_worker_t* worker = mir_worker_get_context(); 
    uint16_t node = runtime->arch->node_of(worker->id);

    // Pop from own node queue
    //MIR_RECORDER_STATE_BEGIN(MIR_STATE_TMOBING);

    struct mir_queue_t* queue = sp->queues[node];
    if(mir_queue_size(queue) > 0)
    {
        mir_queue_pop(queue, (void**)&(*task));
        if(*task)
        {
            // Update stats
            __sync_fetch_and_sub(&g_num_tasks_waiting, 1);
            T_DBG("Dq", *task);
            found = 1;
        }
    }

    //MIR_RECORDER_STATE_END(NULL, 0);

    if (found)
        return found;

    // Next try to pop from other queues
    // Nearest queues get searched first
    // Pop by hop count
    //MIR_RECORDER_STATE_BEGIN(MIR_STATE_TSTEALING);
    
    uint16_t neighbors[runtime->arch->num_nodes];
    uint16_t count;

    for(int d=0; d<runtime->arch->diameter && found!=1; d++)
    {
        count = runtime->arch->vicinity_of(neighbors, node, d);
        for(int i=0; i<count; i++)
        {
            struct mir_queue_t* queue = sp->queues[neighbors[i]];
            if(mir_queue_size(queue) > 0)
            {
                mir_queue_pop(queue, (void**)&(*task));
                if(*task)
                {
                    // Update comm cost
                    struct mir_mem_node_dist_t* dist = mir_task_get_footprint_dist(*task, MIR_DATA_ACCESS_READ);
                    if(dist)
                        (*task)->comm_cost = get_comm_cost(neighbors[i], dist);

                    // Update stats
                    __sync_fetch_and_sub(&g_num_tasks_waiting, 1);
                    worker->status->num_tasks_stolen++;
                    T_DBG("Dq", *task);
                    found = 1;
                    break;
                }
            }
        }
    }

    //MIR_RECORDER_STATE_END(NULL, 0);

    return found;
}/*}}}*/

struct mir_sched_pol_t policy_numa = 
{/*{{{*/
    .num_queues = MIR_WORKER_MAX_COUNT,
    .queue_capacity = MIR_QUEUE_MAX_CAPACITY,
    .queues = NULL,
    .name = "numa",
    .config = config_numa,
    .create = create_numa,
    .destroy = destroy_numa,
    .push = push_numa,
    .pop = pop_numa
};/*}}}*/

