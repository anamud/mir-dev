#include "mir_runtime.h"
#include "scheduling/mir_sched_pol.h"
#include "mir_worker.h"
#include "mir_task.h"
#include "mir_queue.h"
#include "mir_recorder.h"
#include "mir_memory.h"
#include "mir_utils.h"
#include "mir_defines.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

extern uint32_t g_num_tasks_waiting;
extern struct mir_runtime_t* runtime;

void config_ws (const char* conf_str)
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

void create_ws ()
{/*{{{*/
    struct mir_sched_pol_t* sp = runtime->sched_pol;

    // Create worker private task queues
    sp->num_queues = runtime->num_workers;
    sp->queues = (struct mir_queue_t**) mir_malloc_int (sp->num_queues * sizeof(struct mir_queue_t*));
    if(NULL == sp->queues)
        MIR_ABORT(MIR_ERROR_STR "Unable to create task queues!\n");

    for(int i=0; i< sp->num_queues; i++)
        sp->queues[i] = mir_queue_create(sp->queue_capacity);
}/*}}}*/

void destroy_ws ()
{/*{{{*/
    struct mir_sched_pol_t* sp = runtime->sched_pol;
    
    // Free queues
    for(int i=0; i<sp->num_queues ; i++)
        mir_queue_destroy(sp->queues[i]);

    mir_free_int(sp->queues, sizeof(struct mir_queue_t*) * sp->num_queues);
}/*}}}*/

void push_ws (struct mir_task_t* task)
{/*{{{*/
    //MIR_RECORDER_STATE_BEGIN(MIR_STATE_TSCHED);

    // Get this worker!
    struct mir_worker_t* worker = mir_worker_get_context(); 

    // ws has per-worker queues
    struct mir_queue_t* queue = runtime->sched_pol->queues[worker->id];
    if( false == mir_queue_push(queue, (void*) task) )
    {
#ifdef MIR_INLINE_TASK_IF_QUEUE_FULL 
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

    //MIR_RECORDER_STATE_END(NULL, 0);
}/*}}}*/

bool pop_ws (struct mir_task_t** task)
{/*{{{*/
    bool found = 0;
    struct mir_sched_pol_t* sp = runtime->sched_pol;
    uint32_t num_queues = sp->num_queues;
    struct mir_worker_t* worker = mir_worker_get_context(); 
    uint16_t node = runtime->arch->node_of(worker->core_id);

    // First try to pop from own queue
    //MIR_RECORDER_STATE_BEGIN(MIR_STATE_TPOP);

    struct mir_queue_t* queue = sp->queues[worker->id];
    if(mir_queue_size(queue) > 0)
    {
        mir_queue_pop(queue, (void**)&(*task));
        if(*task)
        {
            // Update stats
            if(runtime->enable_stats)
            {
#ifdef MIR_MEM_POL_ENABLE
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
#endif

                worker->status->num_tasks_owned++;
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
    //MIR_RECORDER_STATE_BEGIN(MIR_STATE_TSTEAL);

    uint16_t ctr = worker->id + 1;
    if(ctr == num_queues) ctr = 0;

    while(ctr != worker->id)
    {
        struct mir_queue_t* queue = sp->queues[ctr];
        if(mir_queue_size(queue) > 0)
        {
            mir_queue_pop(queue, (void**)&(*task));
            if(*task) 
            {
                // Update stats
                if(runtime->enable_stats)
                {
#ifdef MIR_MEM_POL_ENABLE
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
#endif

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

    //MIR_RECORDER_STATE_END(NULL, 0);

    return found;
}/*}}}*/

struct mir_sched_pol_t policy_ws = 
{/*{{{*/
    .num_queues = MIR_WORKER_MAX_COUNT,
    .queue_capacity = MIR_QUEUE_MAX_CAPACITY,
    .queues = NULL,
    .alt_queues = NULL,
    .name = "ws",
    .config = config_ws,
    .create = create_ws,
    .destroy = destroy_ws,
    .push = push_ws,
    .pop = pop_ws
};/*}}}*/
