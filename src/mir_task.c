#include "mir_task.h"
#include "mir_defines.h"
#include "mir_types.h"
#include "mir_worker.h"
#include "mir_recorder.h"
#include "mir_runtime.h"
#include "scheduling/mir_sched_pol.h"
#include "mir_utils.h"
#include "mir_memory.h"
#include "mir_queue.h"
#include "mir_loop.h"

#ifdef MIR_MEM_POL_ENABLE
#include "mir_mem_pol.h"
#endif 

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <alloca.h>

extern struct mir_runtime_t* runtime;
extern uint32_t g_num_tasks_waiting;

// FIXME: Make these per-worker
static uint64_t g_tasks_uidc = MIR_TASK_ID_START + 1;

static inline unsigned int mir_twc_reduce(struct mir_twc_t* twc)
{/*{{{*/
    volatile unsigned long sum = 0;
    for(int i=0; i<runtime->num_workers; i++)
        sum += twc->count_per_worker[i];

    // This catches the nasty case of not sychronizing with all tasks previously
    MIR_ASSERT(sum <= twc->count);

    if(sum < twc->count)
        return 0;
    else
        return 1; // sum == twc->count
}/*}}}*/

static inline int inline_necessary()
{/*{{{*/
    if(runtime->task_inlining_limit == 0)
        return 0;

    // Temporary solution to enable inlining unconditionally
    // FIXME: Make this better. Maybe a negative number based condition 
    // ... or based on a number gt num_workers
    if(runtime->task_inlining_limit == 1)
        return 1;

    if(0 == strcmp(runtime->sched_pol->name, "numa"))
        return 0;

    if((g_num_tasks_waiting/runtime->num_workers) >= runtime->task_inlining_limit)
        return 1;

    return 0;
}/*}}}*/

struct mir_task_t* mir_task_create_common(mir_tfunc_t tfunc, void* data, size_t data_size, unsigned int num_data_footprints, const struct mir_data_footprint_t* data_footprints, const char* name)
{/*{{{*/
    MIR_ASSERT(tfunc != NULL);

    // Overhead measurement
    uint64_t start_instant = mir_get_cycles();

    struct mir_task_t* task = NULL;
#ifdef MIR_TASK_ALLOCATE_ON_STACK
    task = (struct mir_task_t*) alloca (sizeof(struct mir_task_t));
#else
    task = (struct mir_task_t*) mir_malloc_int (sizeof(struct mir_task_t));
#endif
    MIR_ASSERT(task != NULL);

    // Task function and argument data
    task->func = tfunc;
    task->data_size = data_size;
    if(data_size > 0)
    {
#ifdef MIR_TASK_FIXED_DATA_SIZE
        MIR_ASSERT(data_size <= MIR_TASK_DATA_MAX_SIZE);
        task->data = &(task->data_buf[0]);
#else
        task->data = mir_malloc_int (sizeof(char) * data_size);
        MIR_ASSERT(task->data != NULL);
#endif
        memcpy((void*)&task->data[0], data, data_size);
    }
    else
        task->data = data; 

    // Task unique id
    // A running number
    task->id.uid = __sync_fetch_and_add(&(g_tasks_uidc), 1);

    // Task name
    MIR_ASSERT(strlen(MIR_TASK_DEFAULT_NAME) < MIR_SHORT_NAME_LEN);
    strcpy(task->name, MIR_TASK_DEFAULT_NAME);
    if(name)
    {/*{{{*/
        MIR_ASSERT(strlen(name) < MIR_SHORT_NAME_LEN);
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
        MIR_ASSERT(task->data_footprints != NULL);

        for (int i=0; i<num_data_footprints; i++)
            data_footprint_copy(&task->data_footprints[i], &data_footprints[i]);

        task->num_data_footprints = num_data_footprints;
    }/*}}}*/

    // Task parent
    struct mir_worker_t* worker = mir_worker_get_context(); 
    MIR_ASSERT(worker != NULL);
    task->parent = worker->current_task;

    // Wait counters
    // For children
    task->ctwc = mir_twc_create();
    // Link to parent wait counter
    if(task->parent)
        task->twc = task->parent->ctwc;
    else
        task->twc = runtime->ctwc;
    __sync_fetch_and_add(&(task->twc->count), 1);
    
    // Task children book-keeping
    task->num_children = 0;
    task->child_number = 0;
    if(task->parent)
    {
        __sync_fetch_and_add(&(task->parent->num_children), 1);
        task->child_number = task->parent->num_children;
    }
    else
    {
        __sync_fetch_and_add(&(runtime->num_children_tasks), 1);
        task->child_number = runtime->num_children_tasks;
    }

    // Other book-keeping
    task->queue_size_at_pop = 0;

    // Flags
    task->done = 0;
    task->taken = 0;

    // Loop
    task->loop = NULL;

    // Overhead measurement
    if(worker->current_task) worker->current_task->overhead_cycles += (mir_get_cycles() - start_instant);

    // Task is now created
    T_DBG("Cr", task);

    return task;
}/*}}}*/

static inline void mir_task_schedule(struct mir_task_t* task)
{/*{{{*/
    MIR_ASSERT(task != NULL);

    // Overhead measurement
    uint64_t start_instant = mir_get_cycles();

    // Push task to the scheduling policy
    int pushed = runtime->sched_pol->push(task);

    // Overhead measurement
    if(pushed == 1)
    {
        struct mir_worker_t* worker = mir_worker_get_context(); MIR_ASSERT(worker != NULL); 
        if(worker->current_task) worker->current_task->overhead_cycles += (mir_get_cycles() - start_instant);
    }

    //__sync_synchronize();
    T_DBG("Sb", task);
}/*}}}*/

static inline void mir_task_schedule_on_worker(struct mir_task_t* task, unsigned int workerid)
{/*{{{*/
    MIR_ASSERT(workerid < runtime->num_workers);
    MIR_ASSERT(task != NULL);

    // Overhead measurement
    uint64_t start_instant = mir_get_cycles();

    // Push task to specific worker 
    struct mir_worker_t* worker = &runtime->workers[workerid];
    MIR_ASSERT(worker != NULL);
    mir_worker_push(worker, task);

    // Overhead measurement
    if(worker->current_task) worker->current_task->overhead_cycles += (mir_get_cycles() - start_instant);

    //__sync_synchronize();
    T_DBG("Sb", task);
}/*}}}*/

void mir_task_create(mir_tfunc_t tfunc, void* data, size_t data_size, unsigned int num_data_footprints, struct mir_data_footprint_t* data_footprints, const char* name)
{/*{{{*/
    MIR_ASSERT(tfunc != NULL);

    // To inline or not to line, that is the grand question!
    if(inline_necessary() == 1)
    {
        tfunc(data);
        // Update worker stats
        struct mir_worker_t* worker = mir_worker_get_context(); 
        MIR_ASSERT(worker != NULL);
        if(runtime->enable_worker_stats == 1)
            worker->statistics->num_tasks_inlined++;
        return;
        // FIXME: What about reporting inlining to the Pin profiler!?
    }

    // Go on and create the task
    MIR_RECORDER_STATE_BEGIN(MIR_STATE_TCREATE);

    // Create task
    struct mir_task_t* task = mir_task_create_common(tfunc, data, data_size, num_data_footprints, data_footprints, name);
    MIR_ASSERT(task != NULL);

    // Schedule task
    mir_task_schedule(task);

    MIR_RECORDER_STATE_END(NULL, 0);
}/*}}}*/

void mir_task_create_on_worker(mir_tfunc_t tfunc, void* data, size_t data_size, unsigned int num_data_footprints, struct mir_data_footprint_t* data_footprints, const char* name, unsigned int workerid)
{/*{{{*/
    MIR_RECORDER_STATE_BEGIN(MIR_STATE_TCREATE);

    // Create task
    struct mir_task_t* task = mir_task_create_common(tfunc, data, data_size, num_data_footprints, data_footprints, name);
    MIR_ASSERT(task != NULL);

    // Schedule task
    mir_task_schedule_on_worker(task, workerid);

    MIR_RECORDER_STATE_END(NULL, 0);
}/*}}}*/

void mir_loop_task_create(mir_tfunc_t tfunc, void* data, struct mir_loop_des_t* loops, int num_loops, const char* name)
{/*{{{*/
    // Create the task
    MIR_RECORDER_STATE_BEGIN(MIR_STATE_TCREATE);

    if(num_loops == 1)
    {
        // Create task on all workers
        int num_workers = runtime->num_workers;
        for(int i=0; i<num_workers; i++)
        {
            // Create task
            struct mir_task_t* task = mir_task_create_common(tfunc, data, 0, 0, NULL, name);
            MIR_ASSERT(task != NULL);

            // Set loop
            task->loop = &loops[0];

            // Schedule on worker
            mir_task_schedule_on_worker(task, i);
        }
    }
    else
    {
        // Create task on all workers
        int num_workers = runtime->num_workers;
        for(int i=0; i<num_workers; i++)
        {
            // Create task
            struct mir_task_t* task = mir_task_create_common(tfunc, data, 0, 0, NULL, name);
            MIR_ASSERT(task != NULL);

            // Set loop
            task->loop = &loops[i];

            // Schedule on worker
            mir_task_schedule_on_worker(task, i);
        }
    }

        MIR_RECORDER_STATE_END(NULL, 0);
}/*}}}*/

static void mir_task_destroy(struct mir_task_t* task)
{/*{{{*/
    // FIXME: Free the task!
    MIR_ASSERT(task != NULL);
}/*}}}*/

void mir_task_execute_prolog(struct mir_task_t* task)
{/*{{{*/
    MIR_ASSERT(task != NULL);

    // Get this worker
    struct mir_worker_t* worker = mir_worker_get_context();
    MIR_ASSERT(worker != NULL);

    if(runtime->enable_recorder == 1)
    {
        // Compose event metadata
        char event_meta_data_pre[MIR_RECORDER_EVENT_META_DATA_MAX_SIZE-1] = {0};
        if(worker->current_task)
        {
            char temp[MIR_LONG_NAME_LEN] = {0};
            sprintf(temp, "%" MIR_FORMSPEC_UL ",%s", worker->current_task->id.uid, worker->current_task->name);
            MIR_ASSERT(strlen(temp) < (MIR_RECORDER_EVENT_META_DATA_MAX_SIZE-1));
            strcpy(event_meta_data_pre, temp);
        }
        else
            sprintf(event_meta_data_pre, "0, NULL");
        // Record event 
        MIR_RECORDER_EVENT(&event_meta_data_pre[0], MIR_RECORDER_EVENT_META_DATA_MAX_SIZE-1);
        // Record state
        MIR_RECORDER_STATE_BEGIN( MIR_STATE_TEXEC);
    }

    // Current task timing
    if(worker->current_task)
        worker->current_task->exec_cycles += (mir_get_cycles() - worker->current_task->exec_resume_instant);

    // Save task context of worker
    task->predecessor = worker->current_task;

    // Update task context of worker
    worker->current_task = task;

    // Current task timing
    task->exec_cycles = 0;
    task->exec_resume_instant = mir_get_cycles();
    task->overhead_cycles = 0;

    // Write task id to shared memory.
    if(runtime->enable_ofp_handshake == 1)
    {
        char buf[MIR_OFP_SHM_SIZE] = {0};
        sprintf(buf, "%" MIR_FORMSPEC_UL, task->id.uid);
        for(int i=0; i<MIR_OFP_SHM_SIZE; i++)
            runtime->ofp_shm[i] = buf[i];
    }
}/*}}}*/

void mir_task_execute_epilog(struct mir_task_t* task)
{/*{{{*/
    MIR_ASSERT(task != NULL);

    // Get this worker
    struct mir_worker_t* worker = mir_worker_get_context();
    MIR_ASSERT(worker != NULL);

    // Record where executed
    task->cpu_id = worker->cpu_id;

    // Current task timing
    task->exec_end_instant = mir_get_cycles() - runtime->init_time;
    task->exec_cycles += (mir_get_cycles() - task->exec_resume_instant);

    // Record sync point 
    // NOTE: All sibling tasks will have the same sync point 
    task->sync_pass = 0;
    if(task->twc)
        task->sync_pass = task->twc->num_passes;

    // Add to task list
    if(runtime->enable_task_stats == 1)
        mir_worker_update_task_list(worker, task);

    // Restore task context of worker
    worker->current_task = task->predecessor;

    // Current task timing
    if(worker->current_task)
        worker->current_task->exec_resume_instant = mir_get_cycles();

    if(runtime->enable_recorder == 1)
    {
        // Record event and state
        char event_meta_data_post[MIR_RECORDER_EVENT_META_DATA_MAX_SIZE-1] = {0};
        char temp[MIR_LONG_NAME_LEN] = {0};
        sprintf(temp, "%" MIR_FORMSPEC_UL ",%s", task->id.uid, task->name);
        MIR_ASSERT(strlen(temp) < (MIR_RECORDER_EVENT_META_DATA_MAX_SIZE-1));
        strcpy(event_meta_data_post, temp);
        MIR_RECORDER_STATE_END(&event_meta_data_post[0], MIR_RECORDER_EVENT_META_DATA_MAX_SIZE-1);
        MIR_RECORDER_EVENT(&event_meta_data_post[0], MIR_RECORDER_EVENT_META_DATA_MAX_SIZE-1);
    }

    // Mark task as done
    task->done = 1;

    // Update task wait counter 
    if(task->twc) 
        task->twc->count_per_worker[worker->id]++;

    // Signal
    T_DBG("Ex", task);
    __sync_synchronize();

    // FIXME Destroy task !
    // NOTE: Destroying task upsets task list structure
}/*}}}*/

void mir_task_execute(struct mir_task_t* task)
{/*{{{*/
    MIR_ASSERT(task != NULL);

    // Start profiling and book-keeping for task
    mir_task_execute_prolog(task);

    // Execute task function
    task->func(task->data);

    // Stop profiling and book-keeping for task
    mir_task_execute_epilog(task);

    // Debugging
    //MIR_INFORM(MIR_INFORM_STR "Task %" MIR_FORMSPEC_UL " executed on worker %d\n", task->id.uid, worker->id);
}/*}}}*/

#ifdef MIR_MEM_POL_ENABLE
static inline void mir_data_footprint_get_mem_node_dist(struct mir_mem_node_dist_t* dist, const struct mir_data_footprint_t* footprint)
{/*{{{*/
    // Check
    MIR_ASSERT(dist != NULL);
    MIR_ASSERT(footprint != NULL);

    uint64_t row_sz = footprint->row_sz;
    if(row_sz > 1)
    {
        for(uint64_t brow=0; brow<=footprint->end; brow++)
        {
            void* base = footprint->base + brow*row_sz*footprint->type;
            mir_mem_get_mem_node_dist( dist, base, 
                    (footprint->end - footprint->start + 1) * footprint->type, 
                    footprint->part_of );
            /*MIR_INFORM("Footprint composed of these addresses:\n");*/
            /*MIR_INFORM("%p[%lu-%lu]\n", base, footprint->start, footprint->end);*/
        }
    }
    else
    {
        mir_mem_get_mem_node_dist( dist, footprint->base, 
                (footprint->end - footprint->start + 1) * footprint->type, 
                footprint->part_of );
        /*MIR_INFORM("Footprint composed of these addresses:\n");*/
        /*MIR_INFORM("%p[%lu-%lu]\n", footprint->base, footprint->start, footprint->end);*/
    }
}/*}}}*/

// FIXME: Multi-tasking fucntion. Seperate!
struct mir_mem_node_dist_t* mir_task_get_mem_node_dist(struct mir_task_t* task, mir_data_access_t access)
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
                mir_data_footprint_get_mem_node_dist(dist, &task->data_footprints[i]);

        // Print dist
        /*MIR_INFORM("Dist for task %" MIR_FORMSPEC_UL ": ", task->id.uid);*/
        /*for(int i=0; i<runtime->arch->num_nodes; i++)*/
            /*MIR_INFORM("%lu ", dist->buf[i]);*/
        /*MIR_INFORM("\n");*/
    }

    return task->dist_by_access_type[access];
}/*}}}*/
#endif

struct mir_twc_t* mir_twc_create() 
{/*{{{*/
    struct mir_twc_t* twc = (struct mir_twc_t*) mir_malloc_int (sizeof(struct mir_twc_t));
    MIR_ASSERT(twc != NULL);

    // Book-keeping
    for(int i=0; i<runtime->num_workers; i++)
       twc->count_per_worker[i] = 0;
    twc->count = 0;

    // Reset num times passed
    twc->num_passes = 0;
    twc->pass_time = (struct mir_time_list_t*) mir_malloc_int (sizeof(struct mir_time_list_t));
    MIR_ASSERT(twc->pass_time != NULL);
    twc->pass_time->next = NULL;

    // Set parent context
    twc->parent = mir_worker_get_context()->current_task;

    return twc;
}/*}}}*/

void mir_task_wait()
{/*{{{*/
    MIR_RECORDER_STATE_BEGIN(MIR_STATE_TSYNC);

    struct mir_worker_t* worker = (struct mir_worker_t*) pthread_getspecific (runtime->worker_index);
    struct mir_twc_t* twc = NULL;
    if(worker->current_task)
        twc = worker->current_task->ctwc;
    else
        twc = runtime->ctwc;

    // Prevent empty synchronizations
    // This upsets finding next forks in the task graph plotter
    if(!(twc->count > 0))
    {
        // TODO: Report this!
        // MIR_ASSERT(twc->count > 0);
        return;
    }

    // Wait and do useful work
    while(mir_twc_reduce(twc) != 1)
    {
        // __sync_synchronize();
        // Sync with or without backoff
        mir_worker_do_work(worker, MIR_WORKER_BACKOFF_DURING_SYNC);
    }

    // Update num times passed
    __sync_fetch_and_add(&(twc->num_passes), 1);

    // Reset counts 
    twc->count = 0;
    for(int i=0; i<runtime->num_workers; i++)
       twc->count_per_worker[i] = 0;

    MIR_RECORDER_STATE_END(NULL, 0);

    return;
}/*}}}*/

void mir_task_list_write_header_to_file(FILE* file)
{/*{{{*/
    fprintf(file, "task,parent,joins_at,cpu_id,child_number,num_children,exec_cycles,overhead_cycles,queue_size,exec_end,tag\n");
}/*}}}*/

void mir_task_list_write_to_file(struct mir_task_list_t* list, FILE* file)
{/*{{{*/
    struct mir_task_list_t* temp = list;
    while(temp != NULL)
    {
        mir_id_t task_parent;
        task_parent.uid = 0;
        if(temp->task->parent)
            task_parent.uid = temp->task->parent->id.uid;

        fprintf(file, "%" MIR_FORMSPEC_UL ",%" MIR_FORMSPEC_UL ",%lu,%u,%u,%u,%" MIR_FORMSPEC_UL ",%" MIR_FORMSPEC_UL ",%u,%" MIR_FORMSPEC_UL ",%s\n", 
                temp->task->id.uid, 
                task_parent.uid,
                temp->task->sync_pass,
                temp->task->cpu_id,
                temp->task->child_number,
                temp->task->num_children,
                temp->task->exec_cycles,
                temp->task->overhead_cycles,
                temp->task->queue_size_at_pop,
                temp->task->exec_end_instant,
                temp->task->name);

        temp = temp->next;
    }
}/*}}}*/

void mir_task_list_destroy(struct mir_task_list_t* list)
{/*{{{*/
    struct mir_task_list_t* temp = list;
    while(temp != NULL)
    {
        struct mir_task_list_t* next = temp->next;
        mir_free_int(temp, sizeof(struct mir_task_list_t));
        temp = next;
        // FIXME: Can also free the task and wait counter here!
    }
}/*}}}*/
