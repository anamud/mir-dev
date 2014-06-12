#include "mir_task.h"
#include "mir_defines.h"
#include "mir_types.h"
#include "mir_worker.h"
#include "mir_recorder.h"
#include "mir_runtime.h"
#include "mir_sched_pol.h"
#include "mir_utils.h"
#include "mir_memory.h"
#include "mir_data_footprint.h"
#include "mir_queue.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <alloca.h>
#include <stdbool.h>

extern struct mir_runtime_t* runtime;
extern uint32_t g_num_tasks_waiting;

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

static inline bool inline_task()
{/*{{{*/
    if(runtime->task_inlining_limit == 0)
        return false;

    // Temporary solution to enable inlining unconditionally
    // FIXME: Make this better. Maybe a negative number based condition 
    // ... or based on a number gt num_workers
    if(runtime->task_inlining_limit == 1)
        return true;

    if(0 == strcmp(runtime->sched_pol->name, "numa"))
        return false;

    if((g_num_tasks_waiting/runtime->num_workers) >= runtime->task_inlining_limit)
        return true;

    return false;
}/*}}}*/

static inline TGPID tgpid_create()
{/*{{{*/
    return mir_malloc_int(sizeof(unsigned int) * TGPID_SIZE);
}/*}}}*/

static inline void tgpid_destroy(TGPID id)
{/*{{{*/
    mir_free_int(id, sizeof(unsigned int) * TGPID_SIZE);
}/*}}}*/

static inline void tgpid_reset(TGPID id)
{/*{{{*/
    memset(id, 0, sizeof(unsigned int) * TGPID_SIZE);
}/*}}}*/

static inline void tgpid_verify(TGPID id)
{/*{{{*/
    // Check if the last two values are 0
    MIR_ASSERT(id[TGPID_SIZE-1] == 0 && id[TGPID_SIZE-2] == 0);
}/*}}}*/

static inline void tgpid_set(TGPID id, struct mir_task_t* parent)
{/*{{{*/
    // Copy parent id shifted by one
    memcpy(id+1, parent->tgpid, sizeof(unsigned int) * TGPID_SIZE);
    // Verify
    tgpid_verify(id);
    // Add child count
    id[0] = parent->num_children;
}/*}}}*/

void tgpid_print(TGPID id, FILE* fp, int cr)
{/*{{{*/
    for(int i=0; i<TGPID_SIZE-2; i++)
    {
        fprintf(fp, "%u.", id[i]);
        if(id[i+1] == 0 && id[i+2] == 0)
            break;
    }
    if(cr == 1)
        fprintf(fp, "\n");
}/*}}}*/

static inline struct mir_task_t* mir_task_create_common(mir_tfunc_t tfunc, void* data, size_t data_size, unsigned int num_data_footprints, struct mir_data_footprint_t* data_footprints, const char* name)
{/*{{{*/
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
#ifdef MIR_TASK_VARIABLE_DATA_SIZE
    task->data = mir_malloc_int (sizeof(char) * data_size);
#else
    MIR_ASSERT(data_size <= MIR_TASK_DATA_MAX_SIZE);
#endif
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

    // Task wait counter
    task->ctwc = mir_twc_create();
    if(task->parent)
        task->twc = task->parent->ctwc;
    else
        task->twc = runtime->ctwc;
    __sync_fetch_and_add(&(task->twc->count), 1);
    
    // Task graph position id
    task->num_children = 0;
    task->tgpid = tgpid_create();
    tgpid_reset(task->tgpid);
    if(task->parent)
    {
        __sync_fetch_and_add(&(task->parent->num_children), 1);
        tgpid_set(task->tgpid, task->parent);
    }

    // Flags
    task->done = 0;
    task->taken = 0;

    // Task is now created
    T_DBG("Cr", task);

    return task;
}/*}}}*/

static inline void mir_task_schedule(struct mir_task_t* task)
{/*{{{*/
    // Push task to the scheduling policy
    runtime->sched_pol->push(task);

    //__sync_synchronize();
    T_DBG("Sb", task);
}/*}}}*/

static inline void mir_task_schedule_on(struct mir_task_t* task, unsigned int target)
{/*{{{*/
    // Push task to target
    // Target = specific worker 
    struct mir_worker_t* worker = &runtime->workers[target];

    mir_worker_push(worker, task);

    //__sync_synchronize();
    T_DBG("Sb", task);
}/*}}}*/

struct mir_task_t* mir_task_create(mir_tfunc_t tfunc, void* data, size_t data_size, unsigned int num_data_footprints, struct mir_data_footprint_t* data_footprints, const char* name)
{/*{{{*/
    // Check if task can be created
    if(runtime->enable_dependence_resolver == 1 && num_data_footprints> 0) 
        MIR_ABORT(MIR_ERROR_STR "Implicit dependence resolution not supported yet!\n");

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

    // Create task
    struct mir_task_t* task = mir_task_create_common(tfunc, data, data_size, num_data_footprints, data_footprints, name);

    // Schedule task
    mir_task_schedule(task);

    MIR_RECORDER_STATE_END(NULL, 0);

    return task;
}/*}}}*/

struct mir_task_t* mir_task_create_on(mir_tfunc_t tfunc, void* data, size_t data_size, unsigned int num_data_footprints, struct mir_data_footprint_t* data_footprints, const char* name, unsigned int target)
{/*{{{*/
    // Check if task can be created
    if(runtime->enable_dependence_resolver == 1 && num_data_footprints> 0) 
        MIR_ABORT(MIR_ERROR_STR "Implicit dependence resolution not supported yet!\n");

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

    // Create task
    struct mir_task_t* task = mir_task_create_common(tfunc, data, data_size, num_data_footprints, data_footprints, name);

    // Schedule task
    mir_task_schedule_on(task, target);

    MIR_RECORDER_STATE_END(NULL, 0);

    return task;
}/*}}}*/

void mir_task_destroy(struct mir_task_t* task)
{/*{{{*/
    // FIXME: Free the task!
}/*}}}*/

void mir_task_execute(struct mir_task_t* task)
{/*{{{*/
    // Get this worker
    struct mir_worker_t* worker = mir_worker_get_context();

    // Compose event metadata
    char event_meta_data[MIR_RECORDER_EVENT_META_DATA_MAX_SIZE-1] = {0};
    //char* event_meta_data = alloca(sizeof(char) * MIR_SHORT_NAME_LEN);
    sprintf(event_meta_data, "%" MIR_FORMSPEC_UL ",%s", task->id.uid, task->name);

    //MIR_DEBUG("%" MIR_FORMSPEC_UL ",%s\n", task->id.uid, task->name);

    // Record event and state
    MIR_RECORDER_EVENT(&event_meta_data[0], MIR_RECORDER_EVENT_META_DATA_MAX_SIZE-1);
    MIR_RECORDER_STATE_BEGIN( MIR_STATE_TEXEC);

    // Save task context of worker
    struct mir_task_t* temp = worker->current_task;

    // Update task context of worker
    worker->current_task = task;

    // Write task id to shared memory.
    // And wait for it to be read
    if(runtime->enable_shmem_handshake == 1)
    {
        char buf[MIR_SHM_SIZE] = {0};
        sprintf(buf, "%" MIR_FORMSPEC_UL, task->id.uid);
        for(int i=0; i<MIR_SHM_SIZE; i++)
            runtime->shm[i] = buf[i];
    }

    //tgpid_print(task->tgpid, stderr, 1);

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

struct mir_twc_t* mir_twc_create() 
{/*{{{*/
    struct mir_twc_t* twc = (struct mir_twc_t*) mir_malloc_int (sizeof(struct mir_twc_t));
    if(twc == NULL)
        MIR_ABORT(MIR_ERROR_STR "Unable to create twc!\n");

   for(int i=0; i<runtime->num_workers; i++)
       twc->count_per_worker[i] = 0;

   twc->count = 0;

    // Reset num times passed
   twc->num_passes = 0;

   twc->parent = mir_worker_get_context()->current_task;

   return twc;
}/*}}}*/

void mir_twc_wait()
{/*{{{*/
    MIR_RECORDER_STATE_BEGIN(MIR_STATE_TSYNC);

    struct mir_worker_t* worker = (struct mir_worker_t*) pthread_getspecific (runtime->worker_index);
    struct mir_twc_t* twc = NULL;
    if(worker->current_task)
        twc = worker->current_task->ctwc;
    else
        twc = runtime->ctwc;

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
