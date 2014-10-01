#include "mir_runtime.h"
#include "scheduling/mir_sched_pol.h"
#include "mir_worker.h"
#include "mir_task.h"
#include "mir_queue.h"
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

#ifdef MIR_MEM_POL_ENABLE
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

    // Create node private alternate task queues
    sp->alt_queues = (struct mir_queue_t**) mir_malloc_int (sp->num_queues * sizeof(struct mir_queue_t*));
    if(NULL == sp->alt_queues)
        MIR_ABORT(MIR_ERROR_STR "Unable to create task alt_queues!\n");

    for(int i=0; i< sp->num_queues; i++)
        sp->alt_queues[i] = mir_queue_create(sp->queue_capacity);
}/*}}}*/

void destroy_numa ()
{/*{{{*/
    struct mir_sched_pol_t* sp = runtime->sched_pol;
    
    // Free queues
    for(int i=0; i<sp->num_queues ; i++)
        mir_queue_destroy(sp->queues[i]);

    mir_free_int(sp->queues, sizeof(struct mir_queue_t*) * sp->num_queues);

    // Free alt_queues
    for(int i=0; i<sp->num_queues ; i++)
        mir_queue_destroy(sp->alt_queues[i]);

    mir_free_int(sp->alt_queues, sizeof(struct mir_queue_t*) * sp->num_queues);
}/*}}}*/

static inline bool is_data_dist_significant(struct mir_mem_node_dist_t* dist)
{/*{{{*/
    // Lower limit by default is the L3 cache available per core
    size_t low_limit = (runtime->arch->llc_size_KB * 1024) / (runtime->arch->num_cores / runtime->arch->num_nodes);
    if(schedule_cutoff_config > 0)
        low_limit = schedule_cutoff_config;

    // Get dist stats
    struct mir_mem_node_dist_stat_t stat;
    mir_mem_node_dist_get_stat(&stat, dist);
    MIR_DEBUG("Dist stats: %lu %lu %lu %.3f %.3f [%lu, 0.0]\n", stat.sum, stat.min, stat.max, stat.mean, stat.sd, low_limit);

    // Check if data from all node is large enough
    if(stat.sum < low_limit)
        return false;

    // FIXME: Refine this!
    if(stat.sd == 0.0)
        return false;

    return true;
}/*}}}*/

void push_numa (struct mir_task_t* task)
{/*{{{*/
    //if(runtime->enable_recorder == 1)
    //MIR_RECORDER_STATE_BEGIN(MIR_STATE_TSCHED);

    struct mir_worker_t* least_cost_worker = NULL;
    struct mir_worker_t* this_worker = mir_worker_get_context();
    bool push_to_alt_queue = false;

    // Push task onto node with the least access cost to read data footprint
    // If no data footprint, push task onto this worker's node
    struct mir_mem_node_dist_t* dist = mir_task_get_mem_node_dist(task, MIR_DATA_ACCESS_READ);
    //struct mir_mem_node_dist_t* dist = mir_task_get_mem_node_dist(task, MIR_DATA_ACCESS_WRITE);
    if(dist)
    {
        if(is_data_dist_significant(dist) == false)
        {
            /*MIR_INFORM("Task %" MIR_FORMSPEC_UL " ignored!\n", task->id.uid);*/
            least_cost_worker = this_worker;
            push_to_alt_queue = true;
            if(runtime->enable_stats)
                task->comm_cost = mir_mem_node_dist_get_comm_cost(dist, runtime->arch->node_of(least_cost_worker->core_id));
        }
        else
        {
            /*MIR_INFORM("Dist for task %" MIR_FORMSPEC_UL ": ", task->id.uid);*/
            /*for(int i=0; i<runtime->arch->num_nodes; i++)*/
                /*MIR_INFORM("%lu ", dist->buf[i]);*/
            /*MIR_INFORM("\n");*/

            uint16_t prev_node = runtime->arch->num_nodes + 1;
            unsigned long least_comm_cost = -1;
            int bias = this_worker->bias;
            int i = bias;
            do
            {
                struct mir_worker_t* worker = &runtime->workers[i];
                uint16_t node = runtime->arch->node_of(worker->core_id);
                if(node != prev_node)
                {
                    prev_node = node;
                    unsigned long comm_cost = mir_mem_node_dist_get_comm_cost(dist, node);
                    if(comm_cost < least_comm_cost)
                    {
                        least_comm_cost = comm_cost;
                        least_cost_worker = worker;
                    }
                }
                i = (i+1)%runtime->num_workers;
            } while (i!=bias);
            mir_worker_update_bias(this_worker);

            if(runtime->enable_stats)
                task->comm_cost = least_comm_cost;

            /*MIR_INFORM("Task %" MIR_FORMSPEC_UL " scheduled on node %d!\n", task->id.uid, runtime->arch->node_of(least_cost_worker->core_id));*/
        }
    }
    else
    {
        least_cost_worker = this_worker;
        push_to_alt_queue = true;
    }

    // Push task to worker's queue
    struct mir_queue_t* queue = NULL;
    if(push_to_alt_queue == true)
        queue = runtime->sched_pol->alt_queues[runtime->arch->node_of(least_cost_worker->core_id)];
    else
        queue = runtime->sched_pol->queues[runtime->arch->node_of(least_cost_worker->core_id)];
    if( false == mir_queue_push(queue, (void*) task) )
    {
#ifdef MIR_INLINE_TASK_IF_QUEUE_FULL 
        mir_task_execute(task);
        // Update stats
        if(runtime->enable_stats)
            this_worker->status->num_tasks_inlined++;
#else
        MIR_ABORT(MIR_ERROR_STR "Cannot enque task. Increase queue capacity using MIR_CONF.\n");
#endif
    }
    else
    {
        //MIR_INFORM(MIR_INFORM_STR "Task scheduled on worker %d\n", least_cost_worker->id);
        __sync_fetch_and_add(&g_num_tasks_waiting, 1);
        
        // Update stats
        if(runtime->enable_stats)
            this_worker->status->num_tasks_created++;
    }

    //if(runtime->enable_recorder == 1)
    //MIR_RECORDER_STATE_END(NULL, 0);
}/*}}}*/

bool pop_numa (struct mir_task_t** task)
{/*{{{*/
    bool found = 0;
    struct mir_sched_pol_t* sp = runtime->sched_pol;
    uint32_t num_queues = sp->num_queues;
    struct mir_worker_t* worker = mir_worker_get_context(); 
    uint16_t node = runtime->arch->node_of(worker->core_id);

    // Pop from own node alt queue
    //if(runtime->enable_recorder == 1)
    //MIR_RECORDER_STATE_BEGIN(MIR_STATE_TPOP);

    struct mir_queue_t* queue = sp->alt_queues[node];
    if(mir_queue_size(queue) > 0)
    {
        mir_queue_pop(queue, (void**)&(*task));
        if(*task)
        {
            // Update stats
            if(runtime->enable_stats)
            {
                // task->comm-cost already calculated in push
                if((*task)->comm_cost != -1)
                {
                    mir_worker_status_update_comm_cost(worker->status, (*task)->comm_cost);
                }

                worker->status->num_tasks_owned++;
            }

            __sync_fetch_and_sub(&g_num_tasks_waiting, 1);
            T_DBG("Dq", *task);

            found = 1;
        }
    }

    //if(runtime->enable_recorder == 1)
    //MIR_RECORDER_STATE_END(NULL, 0);

    if (found)
        return found;

    // Pop from own node queue
    //if(runtime->enable_recorder == 1)
    //MIR_RECORDER_STATE_BEGIN(MIR_STATE_TPOP);

    queue = sp->queues[node];
    if(mir_queue_size(queue) > 0)
    {
        mir_queue_pop(queue, (void**)&(*task));
        if(*task)
        {
            // Update stats
            if(runtime->enable_stats)
            {
                // task->comm-cost already calculated in push
                if((*task)->comm_cost != -1)
                {
                    mir_worker_status_update_comm_cost(worker->status, (*task)->comm_cost);
                }

                worker->status->num_tasks_owned++;
            }

            __sync_fetch_and_sub(&g_num_tasks_waiting, 1);
            T_DBG("Dq", *task);

            found = 1;
        }
    }

    //if(runtime->enable_recorder == 1)
    //MIR_RECORDER_STATE_END(NULL, 0);

    if (found)
        return found;

    // Next try to pop from other queues
    // First check in alt queues

#if 1
    //if(runtime->enable_recorder == 1)
    //MIR_RECORDER_STATE_BEGIN(MIR_STATE_TSTEAL);
    uint16_t ctr = node + 1;
    if(ctr == num_queues) ctr = 0;

    while(ctr != node)
    {
        struct mir_queue_t* queue = sp->alt_queues[ctr];
        if(mir_queue_size(queue) > 0)
        {
            mir_queue_pop(queue, (void**)&(*task));
            if(*task) 
            {
                // Update stats
                if(runtime->enable_stats)
                {
                    struct mir_mem_node_dist_t* dist = mir_task_get_mem_node_dist(*task, MIR_DATA_ACCESS_READ);
                    if(dist)
                    {
                        (*task)->comm_cost = mir_mem_node_dist_get_comm_cost(dist, node);
                        mir_worker_status_update_comm_cost(worker->status, (*task)->comm_cost);
                    }

                    worker->status->num_tasks_stolen++;
                }

                __sync_fetch_and_sub(&g_num_tasks_waiting, 1);
                T_DBG("St", *task);

                found = 1;
                break;
            }
        }

        // Incremenet counter and wrap around
        ctr++;
        if(ctr == num_queues) ctr = 0;
    }

    //if(runtime->enable_recorder == 1)
    //MIR_RECORDER_STATE_END(NULL, 0);
#endif

    if (found)
        return found;

    // Now check in other queues
    // Nearest queues get searched first
    // Pop by hop count
    
#if 1
    //if(runtime->enable_recorder == 1)
    //MIR_RECORDER_STATE_BEGIN(MIR_STATE_TSTEAL);
    uint16_t neighbors[runtime->arch->num_nodes];
    uint16_t count;

    for(int d=1; d<=runtime->arch->diameter && found!=1; d++)
    {/*{{{*/
        count = runtime->arch->vicinity_of(neighbors, node, d);
        for(int i=0; i<count; i++)
        {
            // FIXME: Decision bias possible here
            struct mir_queue_t* queue = sp->queues[neighbors[i]];
            uint32_t queue_sz = mir_queue_size(queue);
            size_t low_limit = (runtime->arch->num_cores/runtime->arch->num_nodes)*d;
            //size_t low_limit = (runtime->arch->num_cores/runtime->arch->num_nodes);
            //size_t low_limit = 0;
            if(queue_sz > low_limit)
            {
                mir_queue_pop(queue, (void**)&(*task));
                if(*task)
                {
                    // Update stats
                    if(runtime->enable_stats)
                    {
                        struct mir_mem_node_dist_t* dist = mir_task_get_mem_node_dist(*task, MIR_DATA_ACCESS_READ);
                        //struct mir_mem_node_dist_t* dist = mir_task_get_mem_node_dist(*task, MIR_DATA_ACCESS_WRITE);
                        if(dist)
                        {
                            (*task)->comm_cost = mir_mem_node_dist_get_comm_cost(dist, node);
                            mir_worker_status_update_comm_cost(worker->status, (*task)->comm_cost);
                            worker->status->num_comm_tasks_stolen_by_diameter[d-1]++;
                        }

                        worker->status->num_tasks_stolen++;
                    }

                    __sync_fetch_and_sub(&g_num_tasks_waiting, 1);
                    T_DBG("Dq", *task);

                    found = 1;
                    break;
                }
            }
        }
    }/*}}}*/

    //if(runtime->enable_recorder == 1)
    //MIR_RECORDER_STATE_END(NULL, 0);
#endif

    return found;
}/*}}}*/

struct mir_sched_pol_t policy_numa = 
{/*{{{*/
    .num_queues = MIR_WORKER_MAX_COUNT,
    .queue_capacity = MIR_QUEUE_MAX_CAPACITY,
    .queues = NULL,
    .alt_queues = NULL,
    .name = "numa",
    .config = config_numa,
    .create = create_numa,
    .destroy = destroy_numa,
    .push = push_numa,
    .pop = pop_numa
};/*}}}*/
#endif

