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

extern struct mir_runtime_t* runtime;

static uint64_t g_tasks_uidc = MIR_TASK_ID_START + 1;

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

struct mir_task_t* mir_task_create(mir_tfunc_t tfunc, void* data, size_t data_size, struct mir_twc_t* twc, unsigned int num_data_footprints, struct mir_data_footprint_t* data_footprints, const char* name)
{/*{{{*/
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
    task->name[0] = '\0';
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
        task->twc->count++;
    }

    // Communication cost
    // Initially communication cost is unknown
    // Is determined when task is scheduled
    task->comm_cost = -1;

    // Data footprint
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
    }/*}}}*/

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
    char event_meta_data[MIR_SHORT_NAME_LEN] = {0};
#ifdef __tile__
    sprintf(event_meta_data, "%llu,%s", task->id.uid, task->name);
#else
    sprintf(event_meta_data, "%lu,%s", task->id.uid, task->name);
#endif

    // Record event and state
    MIR_RECORDER_EVENT(&event_meta_data[0], 16);
    MIR_RECORDER_STATE_BEGIN( MIR_STATE_TEXEC);

    // Execute task function
    task->func(task->data);

    // Record event and state
    MIR_RECORDER_STATE_END(&event_meta_data[0], 16);
    MIR_RECORDER_EVENT(&event_meta_data[0], 16);

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

void mir_task_wait(struct mir_task_t* task)
{/*{{{*/
    MIR_RECORDER_STATE_BEGIN(MIR_STATE_TSYNC);

    struct mir_worker_t* worker = mir_worker_get_context();

    // Wait and do useful work
    while(task->done != 1)
    {
        __sync_synchronize();
        T_DBG("Wa", task);
        mir_worker_do_work(worker);
    }

    MIR_RECORDER_STATE_END(NULL, 0);
    return;
}/*}}}*/

void mir_task_wait_twc(struct mir_twc_t* twc)
{/*{{{*/
    MIR_RECORDER_STATE_BEGIN(MIR_STATE_TSYNC);

    struct mir_worker_t* worker = (struct mir_worker_t*) pthread_getspecific (runtime->worker_index);

    // Wait and do useful work
    while(mir_twc_reduce(twc) != 1)
    {
        // __sync_synchronize();
        mir_worker_do_work(worker);
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

   return twc;
}/*}}}*/

