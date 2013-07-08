#include "mir_task.h"
#include "mir_defines.h"
#include "mir_types.h"
#include "mir_worker.h"
#include "mir_recorder.h"
#include "mir_runtime.h"
#include "mir_sched_pol.h"
#include "mir_debug.h"
#include "mir_memory.h"
#include "mir_data_footprint.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <alloca.h>
#include <stdbool.h>

extern struct mir_runtime_t* runtime;
extern uint32_t g_num_tasks_waiting;

static uint64_t g_tasks_uidc = MIR_TASK_ID_START + 1;
static uint64_t g_taskwaits_uidc = MIR_TASKWAIT_ID_START + 1;

static inline unsigned int mir_twc_reduce(struct mir_twc_t* twc)
{/*{{{*/
    volatile unsigned long sum = 0;
    for(int i=0; i<runtime->num_workers; i++)
        sum += twc->count_per_worker[i];

    if(sum < twc->count)
        return 0;
    else
        return 1;
}/*}}}*/

static inline bool inline_task()
{/*{{{*/
    if(runtime->task_inlining_limit == 0)
        return false;

    if(0 == strcmp(runtime->sched_pol->name, "numa"))
        return false;

    if((g_num_tasks_waiting/runtime->num_workers) >= runtime->task_inlining_limit)
        return true;

    return false;
}/*}}}*/

struct mir_task_t* mir_task_create(mir_tfunc_t tfunc, void* data, size_t data_size, struct mir_twc_t* twc, unsigned int num_data_footprints, struct mir_data_footprint_t* data_footprints, const char* name)
{/*{{{*/
    // To inline or not to line, that is the grand question!
    if(inline_task())
    {
        tfunc(data);
        // Update stats
        struct mir_worker_t* worker = mir_worker_get_context(); 
        if(runtime->enable_stats)
            worker->status->num_tasks_inlined++;
        return NULL;
    }

    // Go on and create the task
    MIR_RECORDER_STATE_BEGIN(MIR_STATE_TCREATE);

    struct mir_task_t* task = NULL;
#ifdef MIR_TASK_ALLOCATE_ON_STACK
    task = (struct mir_task_t*) alloca (sizeof(struct mir_task_t));
#else
    task = (struct mir_task_t*) mir_malloc_int (sizeof(struct mir_task_t));
#endif
    if(task == NULL)
        MIR_ABORT(MIR_ERROR_STR "Could not allocate memory!\n");

    // Task function and argument data
    task->func = tfunc;
    MIR_ASSERT(data_size <= MIR_BA_SBUF_SIZE);
    task->data_size = data_size;
    memcpy((void*)&task->data[0], data, data_size);

    // Task unique id
    // A running number
    task->id.uid = __sync_fetch_and_add(&(g_tasks_uidc), 1);

    // Task name
    strcpy(task->name, MIR_TASK_DEFAULT_NAME);
    if(name)
    {/*{{{*/
        if(strlen(name) > MIR_SHORT_NAME_LEN)
            MIR_ABORT(MIR_ERROR_STR "Task name longer than %d characters!\n", MIR_SHORT_NAME_LEN);
        else
            strcpy(task->name, name);
    }/*}}}*/

    // Task wait counter
    task->twc = NULL;
    if(twc) 
    {
        task->twc = twc;
        __sync_fetch_and_add(&(task->twc->count), 1);
    }

    // Communication cost
    // Initially communication cost is unknown
    // Is determined when task is scheduled
    task->comm_cost = -1;

    // Data footprint
    for(int i=0; i<MIR_DATA_ACCESS_NUM_TYPES; i++)
        task->dist_by_access_type[i] = NULL;
    task->num_data_footprints = 0;
    task->data_footprints = NULL;
    if (num_data_footprints > 0)
    {/*{{{*/
        // FIXME: Dynamic allocation increases task creation time
#ifdef MIR_TASK_ALLOCATE_ON_STACK
        task->data_footprints = ( struct mir_data_footprint_t* ) alloca ( num_data_footprints * sizeof( struct mir_data_footprint_t ) );
#else
        task->data_footprints = ( struct mir_data_footprint_t* ) mir_malloc_int ( num_data_footprints * sizeof( struct mir_data_footprint_t ) );
#endif
        if(task->data_footprints == NULL)
            MIR_ABORT(MIR_ERROR_STR "Could not allocate memory!\n");

        for (int i=0; i<num_data_footprints; i++)
        {
            mir_data_footprint_copy(&task->data_footprints[i], &data_footprints[i]);
        }

        task->num_data_footprints = num_data_footprints;
    }/*}}}*/

    // Task parent
    struct mir_worker_t* worker = mir_worker_get_context(); 
    task->parent = worker->current_task;

    // Flags
    task->done = 0;

    // Task is now created
    T_DBG("Cr", task);

    MIR_RECORDER_STATE_END(NULL, 0);

    // Schedule task
    mir_task_schedule(task);

    return task;
}/*}}}*/

void mir_task_destroy(struct mir_task_t* task)
{/*{{{*/
    // FIXME: Free the task!
}/*}}}*/

void mir_task_schedule(struct mir_task_t* task)
{/*{{{*/
    // Check if task can be scheduled
    // Analyze dependences
    if(runtime->enable_dependence_resolver == 1 && task->num_data_footprints> 0) 
    {
        MIR_ABORT(MIR_ERROR_STR "Dependence reolver not supported yet!\n");
    } 
    else 
        mir_task_schedule_int(task);
}/*}}}*/

void mir_task_schedule_int(struct mir_task_t* task)
{/*{{{*/
    // Get this worker
    struct mir_worker_t* worker = mir_worker_get_context();

    // Push task to the scheduling policy
    runtime->sched_pol->push(task);

    //__sync_synchronize();
    T_DBG("Sb", task);
}/*}}}*/

void mir_task_execute(struct mir_task_t* task)
{/*{{{*/
    // Get this worker
    struct mir_worker_t* worker = mir_worker_get_context();

    // Compose event metadata
    char event_meta_data[MIR_RECORDER_EVENT_META_DATA_MAX_SIZE-1] = {0};
    //char* event_meta_data = alloca(sizeof(char) * MIR_SHORT_NAME_LEN);
    sprintf(event_meta_data, "%" MIR_FORMSPEC_UL ",%s", task->id.uid, task->name);

    // Record event and state
    MIR_RECORDER_EVENT(&event_meta_data[0], MIR_RECORDER_EVENT_META_DATA_MAX_SIZE-1);
    MIR_RECORDER_STATE_BEGIN( MIR_STATE_TEXEC);

    // Save task context of worker
    struct mir_task_t* temp = worker->current_task;

    // Update task context of worker
    worker->current_task = task;

    // Execute task function
    task->func(task->data);

    // Add to task graph
    mir_worker_update_task_graph(worker, task);

    // Restore task context of worker
    worker->current_task = temp;

    //MIR_INFORM(MIR_INFORM_STR "Task %" MIR_FORMSPEC_UL " executed on worker %d\n", task->id.uid, worker->id);

    // Record event and state
    MIR_RECORDER_STATE_END(&event_meta_data[0], MIR_RECORDER_EVENT_META_DATA_MAX_SIZE-1);
    MIR_RECORDER_EVENT(&event_meta_data[0], MIR_RECORDER_EVENT_META_DATA_MAX_SIZE-1);

    // Mark task as done
    task->done = 1;

    // Update task wait counter 
    if(task->twc) 
        task->twc->count_per_worker[worker->id]++;

    // Signal
    T_DBG("Ex", task);
    __sync_synchronize();

    // FIXME Destroy task !
}/*}}}*/

struct mir_mem_node_dist_t* mir_task_get_footprint_dist(struct mir_task_t* task, mir_data_access_t access)
{/*{{{*/
    if(task->num_data_footprints == 0)
        return NULL;

    if(task->dist_by_access_type[access] == NULL)
    {
        // Allocate dist
        struct mir_mem_node_dist_t* dist = mir_mem_node_dist_create();
        task->dist_by_access_type[access] = dist;

        // Calculate dist
        for(int i=0; i<task->num_data_footprints; i++)
            if(task->data_footprints[i].data_access == access)
                mir_data_footprint_get_dist(dist, &task->data_footprints[i]);

        // Print dist
        /*MIR_INFORM("Dist for task %" MIR_FORMSPEC_UL ": ", task->id.uid);*/
        /*for(int i=0; i<runtime->arch->num_nodes; i++)*/
            /*MIR_INFORM("%lu ", dist->buf[i]);*/
        /*MIR_INFORM("\n");*/
    }

    return task->dist_by_access_type[access];
}/*}}}*/

void mir_task_wait(struct mir_task_t* task)
{/*{{{*/
    if(!task)
        return;

    MIR_RECORDER_STATE_BEGIN(MIR_STATE_TSYNC);

    struct mir_worker_t* worker = mir_worker_get_context();

    // Wait and do useful work
    while(task->done != 1)
    {
        __sync_synchronize();
        T_DBG("Wa", task);
#ifdef MIR_WORKER_BACKOFF_DURING_SYNC
    // Sync with backoff=true
    mir_worker_do_work(worker, true);
#else
    // Sync with backoff=false
    mir_worker_do_work(worker, false);
#endif
    }

    MIR_RECORDER_STATE_END(NULL, 0);
    return;
}/*}}}*/

struct mir_twc_t* mir_twc_create() 
{/*{{{*/
    struct mir_twc_t* twc = (struct mir_twc_t*) mir_malloc_int (sizeof(struct mir_twc_t));
    if(twc == NULL)
        MIR_ABORT(MIR_ERROR_STR "Unable to create twc!\n");

   for(int i=0; i<runtime->num_workers; i++)
       twc->count_per_worker[i] = 0;

   twc->count = 0;

    // Task wait unique id
    // A running number
    twc->id.uid = __sync_fetch_and_add(&(g_taskwaits_uidc), 1);

    // Reset num times passed
   twc->num_passes = 0;

   twc->parent = mir_worker_get_context()->current_task;

   return twc;
}/*}}}*/

void mir_twc_wait(struct mir_twc_t* twc)
{/*{{{*/
    MIR_RECORDER_STATE_BEGIN(MIR_STATE_TSYNC);

    struct mir_worker_t* worker = (struct mir_worker_t*) pthread_getspecific (runtime->worker_index);

    // Wait and do useful work
    while(mir_twc_reduce(twc) != 1)
    {
        // __sync_synchronize();
#ifdef MIR_WORKER_BACKOFF_DURING_SYNC
        // Sync with backoff=true
        mir_worker_do_work(worker, true);
#else
        // Sync with backoff=false
        mir_worker_do_work(worker, false);
#endif
    }

    // Update num times passed
    twc->num_passes++;

    MIR_RECORDER_STATE_END(NULL, 0);
    return;
}/*}}}*/

