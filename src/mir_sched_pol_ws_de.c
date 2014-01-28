#include "mir_runtime.h"
#include "mir_sched_pol.h"
#include "mir_worker.h"
#include "mir_task.h"
#include "mir_queue.h"
#include "mir_dequeue.h"
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

void config_ws_de (const char* conf_str)
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
            {
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
                default:
                    break;
            }
        }
        tok = strtok(NULL, " ");
    }
}/*}}}*/

void create_ws_de ()
{/*{{{*/
    struct mir_sched_pol_t* sp = runtime->sched_pol;

    // Create worker private task queues
    sp->num_queues = runtime->num_workers;
    sp->queues = (struct mir_queue_t**) mir_malloc_int (sp->num_queues * sizeof(mir_dequeue_t*));
    if(NULL == sp->queues)
        MIR_ABORT(MIR_ERROR_STR "Unable to create task dequeues!\n");

    for(int i=0; i< sp->num_queues; i++)
        sp->queues[i] = (struct mir_queue_t*) newWSDeque(sp->queue_capacity);
}/*}}}*/

void destroy_ws_de ()
{/*{{{*/
    struct mir_sched_pol_t* sp = runtime->sched_pol;
    
    // Free queues
    for(int i=0; i<sp->num_queues ; i++)
        freeWSDeque((mir_dequeue_t*)sp->queues[i]);

    mir_free_int(sp->queues, sizeof(mir_dequeue_t*) * sp->num_queues);
}/*}}}*/

void push_ws_de (struct mir_task_t* task)
{/*{{{*/
    MIR_RECORDER_STATE_BEGIN(MIR_STATE_TSCHED);

    // Get this worker!
    struct mir_worker_t* worker = mir_worker_get_context(); 

    // ws has per-worker queues
    mir_dequeue_t* queue = (mir_dequeue_t*) runtime->sched_pol->queues[worker->id];
    if( rtsFalse == pushWSDeque(queue, (void*) task) )
    {
#ifdef MIR_SCHED_POL_INLINE_TASKS
        mir_task_execute(task);
        // Update stats
        if(runtime->enable_stats)
            worker->status->num_tasks_inlined++;
#else
        MIR_ABORT(MIR_ERROR_STR "Cannot enque task. Increase queue capacity using MIR_CONF.\n");
#endif
    }
    else
    {
        __sync_fetch_and_add(&g_num_tasks_waiting, 1);
        // Update stats
        if(runtime->enable_stats)
            worker->status->num_tasks_created++;
    }

    MIR_RECORDER_STATE_END(NULL, 0);
}/*}}}*/

bool pop_ws_de (struct mir_task_t** task)
{/*{{{*/
    bool found = 0;
    struct mir_sched_pol_t* sp = runtime->sched_pol;
    uint32_t num_queues = sp->num_queues;
    struct mir_worker_t* worker = mir_worker_get_context(); 
    uint16_t node = runtime->arch->node_of(worker->id);

    // First try to pop from own queue
    //MIR_RECORDER_STATE_BEGIN(MIR_STATE_TPOP);

    mir_dequeue_t* queue = (mir_dequeue_t*) sp->queues[worker->id];
    if(looksEmptyWSDeque(queue) == rtsFalse)
    {
        *task = (struct mir_task_t*) popWSDeque(queue);
        if(*task)
        {
            // Update stats
            if(runtime->enable_stats)
            {
                struct mir_mem_node_dist_t* dist = mir_task_get_footprint_dist(*task, MIR_DATA_ACCESS_READ);
                if(dist)
                {
                    /*MIR_INFORM("Dist for task %" MIR_FORMSPEC_UL ": ", (*task)->id.uid);*/
                    /*for(int i=0; i<runtime->arch->num_nodes; i++)*/
                        /*MIR_INFORM("%lu ", dist->buf[i]);*/
                    /*MIR_INFORM("\n");*/

                    (*task)->comm_cost = mir_sched_pol_get_comm_cost(node, dist);
                    mir_worker_status_update_comm_cost(worker->status, (*task)->comm_cost);
                }

                worker->status->num_tasks_owned++;
            }

            if(g_num_tasks_waiting > 0)
                __sync_fetch_and_sub(&g_num_tasks_waiting, 1);
            T_DBG("Dq", *task);

            found = 1;
        }
    }

    //MIR_RECORDER_STATE_END(NULL, 0);

    if (found)
        return found;

    // Next try to pop from other queues
    //MIR_RECORDER_STATE_BEGIN(MIR_STATE_TSTEAL);

    uint16_t ctr = worker->id + 1;
    if(ctr == num_queues) ctr = 0;

    while(ctr != worker->id)
    {
        mir_dequeue_t* queue = (mir_dequeue_t*) sp->queues[ctr];
        if(looksEmptyWSDeque(queue) == rtsFalse)
        {
            *task = (struct mir_task_t*) stealWSDeque(queue);
            if(*task) 
            {
                // Update stats
                if(runtime->enable_stats)
                {
                    struct mir_mem_node_dist_t* dist = mir_task_get_footprint_dist(*task, MIR_DATA_ACCESS_READ);
                    if(dist)
                    {
                        /*MIR_INFORM("Dist for task %" MIR_FORMSPEC_UL ": ", (*task)->id.uid);*/
                        /*for(int i=0; i<runtime->arch->num_nodes; i++)*/
                            /*MIR_INFORM("%lu ", dist->buf[i]);*/
                        /*MIR_INFORM("\n");*/

                        (*task)->comm_cost = mir_sched_pol_get_comm_cost(node, dist);
                        mir_worker_status_update_comm_cost(worker->status, (*task)->comm_cost);
                    }

                    worker->status->num_tasks_stolen++;
                }

                if(g_num_tasks_waiting > 0)
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

    //MIR_RECORDER_STATE_END(NULL, 0);

    return found;
}/*}}}*/

struct mir_sched_pol_t policy_ws_de = 
{/*{{{*/
    .num_queues = MIR_WORKER_MAX_COUNT,
    .queue_capacity = MIR_QUEUE_MAX_CAPACITY,
    .queues = NULL,
    .alt_queues = NULL,
    .name = "ws-de",
    .config = config_ws_de,
    .create = create_ws_de,
    .destroy = destroy_ws_de,
    .push = push_ws_de,
    .pop = pop_ws_de
};/*}}}*/

