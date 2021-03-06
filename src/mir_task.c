#include "mir_task.h"
#include "mir_defines.h"
#include "mir_types.h"
#include "mir_worker.h"
#include "mir_recorder.h"
#include "mir_runtime.h"
#include "scheduling/mir_sched_pol.h"
#include "mir_utils.h"
#include "mir_memory.h"
#include "mir_task_queue.h"
#include "mir_loop.h"
#include "mir_mem_pol.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <alloca.h>

extern uint32_t g_num_tasks_waiting;

// FIXME: Make these per-worker
uint64_t g_tasks_uidc = MIR_TASK_ID_START;

static inline unsigned int mir_twc_reduce(struct mir_twc_t* twc)
{ /*{{{*/
    volatile unsigned long sum = 0;
    for (int i = 0; i < runtime->num_workers; i++)
        sum += twc->count_per_worker[i];

    // This catches the nasty case of not sychronizing with all tasks previously
    MIR_ASSERT(sum <= twc->count);

    if (sum < twc->count)
        return 0;
    else
        return 1; // sum == twc->count
} /*}}}*/

static inline uint64_t elapsed_execution_time(struct mir_task_t* task)
{ /*{{{*/
    MIR_ASSERT(task != NULL);
    return task->exec_cycles + (mir_get_cycles() - task->exec_resume_instant);
} /*}}}*/

static inline int inline_necessary()
{ /*{{{*/
    if (runtime->task_inlining_limit == 0)
        return 0;

    // Temporary solution to enable inlining unconditionally
    // FIXME: Make this better. Maybe a negative number based condition
    // ... or based on a number gt num_workers
    if (runtime->task_inlining_limit == 1)
        return 1;

    if (0 == strcmp(runtime->sched_pol->name, "numa"))
        return 0;

    if ((g_num_tasks_waiting / runtime->num_workers) >= runtime->task_inlining_limit)
        return 1;

    return 0;
} /*}}}*/

struct mir_task_t* mir_task_create_twin(char *name, struct mir_task_t* task, char *str)
{/*{{{*/
    MIR_ASSERT(task != NULL);
    struct mir_task_t* twin;
    twin = mir_task_create_common(task->func, task->data, task->data_size, task->num_data_footprints, task->data_footprints, name, task->team, task->loop, task->parent);
    mir_task_write_metadata(twin, str);

    return twin;
}/*}}}*/

struct mir_task_t* mir_task_create_common(mir_tfunc_t tfunc, void* data, size_t data_size, unsigned int num_data_footprints, const struct mir_data_footprint_t* data_footprints, const char* name, struct mir_omp_team_t* myteam, struct mir_loop_des_t* loopdes, struct mir_task_t* parent)
{ /*{{{*/
    MIR_ASSERT(tfunc != NULL);

    // Overhead measurement
    uint64_t start_instant = mir_get_cycles();

    struct mir_task_t* task;
#ifdef MIR_TASK_ALLOCATE_ON_STACK
    task = alloca(sizeof(struct mir_task_t));
#else
    task = mir_malloc_int(sizeof(struct mir_task_t));
#endif
    MIR_CHECK_MEM(task != NULL);

    // Task function and argument data
    task->func = tfunc;
    task->data_size = data_size;
    if (data_size > 0) {
#ifdef MIR_TASK_FIXED_DATA_SIZE
        MIR_ASSERT(data_size <= MIR_TASK_DATA_MAX_SIZE);
        task->data = &(task->data_buf[0]);
#else
        task->data = mir_malloc_int(sizeof(char) * data_size);
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
    if (name) { /*{{{*/
        MIR_ASSERT_STR(strlen(name) < MIR_SHORT_NAME_LEN, "Task name cannot be larger than %d characters.", MIR_SHORT_NAME_LEN);
        strcpy(task->name, name);
    } /*}}}*/

    // Task metadata
    mir_task_write_metadata(task, NULL);

    // Communication cost
    // Initially communication cost is unknown
    // Is determined when task is scheduled
    task->comm_cost = -1;

    // Data footprint
    for (int i = 0; i < MIR_DATA_ACCESS_NUM_TYPES; i++)
        task->dist_by_access_type[i] = NULL;
    task->num_data_footprints = 0;
    task->data_footprints = NULL;
    if (num_data_footprints > 0) { /*{{{*/
                                   // FIXME: Dynamic allocation increases task creation time
#ifdef MIR_TASK_ALLOCATE_ON_STACK
        task->data_footprints = alloca(num_data_footprints * sizeof(struct mir_data_footprint_t));
#else
        task->data_footprints = mir_malloc_int(num_data_footprints * sizeof(struct mir_data_footprint_t));
#endif
        MIR_CHECK_MEM(task->data_footprints != NULL);

        for (int i = 0; i < num_data_footprints; i++)
            data_footprint_copy(&task->data_footprints[i], &data_footprints[i]);

        task->num_data_footprints = num_data_footprints;
    } /*}}}*/

    // Task parent
    task->parent = parent;
    task->team = myteam;

    // Wait counters
    // For children
    task->ctwc = mir_twc_create();
    // Link to parent wait counter
    if (parent)
        task->twc = parent->ctwc;
    else
        task->twc = runtime->ctwc;
    __sync_fetch_and_add(&(task->twc->count), 1);

    // Task children book-keeping
    task->num_children = 0;
    task->child_number = 0;
    if (parent) {
        __sync_fetch_and_add(&(parent->num_children), 1);
        task->child_number = parent->num_children;
    }
    else {
        __sync_fetch_and_add(&(runtime->num_children_tasks), 1);
        task->child_number = runtime->num_children_tasks;
    }

    // Other book-keeping
    task->queue_size_at_pop = 0;

    // Flags
    task->done = 0;
    task->taken = 0;

    // Create loop structure to support GOMP_loop_*_start.
    task->loop = loopdes;

    // Creation cost
    task->creation_cycles = (mir_get_cycles() - start_instant);

    // Overhead measurement
    if (parent)
        parent->overhead_cycles += task->creation_cycles;

    // Record creation instant
    task->create_instant = 0;
    if (parent)
        task->create_instant = elapsed_execution_time(parent);

    // Task is now created
    T_DBG("Cr", task);

    return task;
} /*}}}*/

void mir_task_schedule_on_worker(struct mir_task_t* task, int workerid)
{ /*{{{*/
    MIR_ASSERT(workerid < runtime->num_workers);
    MIR_ASSERT(task != NULL);

    // Overhead measurement
    uint64_t start_instant = mir_get_cycles();

    // Worker is this worker.
    struct mir_worker_t* worker = mir_worker_get_context();
    MIR_ASSERT(worker != NULL);

    int pushed;

    if (workerid < 0) {
        // Push task to the scheduling policy
        pushed = runtime->sched_pol->push(worker, task);
    }
    else {
        struct mir_worker_t* to_worker = &runtime->workers[workerid];
        MIR_ASSERT(to_worker != NULL);
        // Push task to specific worker
        mir_worker_push(to_worker, task);
        pushed = 1;
    }

    // Overhead measurement
    if (pushed == 1 && worker->current_task)
        worker->current_task->overhead_cycles += (mir_get_cycles() - start_instant);

    //__sync_synchronize();
    T_DBG("Sb", task);
} /*}}}*/

void mir_task_create(mir_tfunc_t tfunc, void* data, size_t data_size, unsigned int num_data_footprints, struct mir_data_footprint_t* data_footprints, const char* name)
{ /*{{{*/
    MIR_ASSERT(tfunc != NULL);

    mir_task_create_on_worker(tfunc, data, data_size, num_data_footprints,
                              data_footprints, name, NULL, NULL, -1);
} /*}}}*/

void mir_task_create_on_worker(mir_tfunc_t tfunc, void* data, size_t data_size, unsigned int num_data_footprints, struct mir_data_footprint_t* data_footprints, const char* name, struct mir_omp_team_t* myteam, struct mir_loop_des_t* loopdes, int workerid)
{ /*{{{*/
    // To inline or not to line, that is the grand question!
    if (workerid < 0 && inline_necessary() == 1) {
        MIR_CONTEXT_EXIT;

        tfunc(data);

        MIR_CONTEXT_ENTER;

        // Update worker stats
        if (runtime->enable_worker_stats == 1) {
            struct mir_worker_t* worker = mir_worker_get_context();
            MIR_ASSERT(worker != NULL);
            worker->statistics->num_tasks_inlined++;
        }
        return;
        // FIXME: What about reporting inlining to the Pin profiler!?
    }

    MIR_RECORDER_STATE_BEGIN(MIR_STATE_TCREATE);

    // Get this worker
    struct mir_worker_t* worker = mir_worker_get_context();
    MIR_ASSERT(worker != NULL);

    // Create task
    struct mir_task_t* task = mir_task_create_common(tfunc, data, data_size, num_data_footprints, data_footprints, name, myteam, loopdes, worker->current_task);
    MIR_CHECK_MEM(task != NULL);

    // Schedule task
    mir_task_schedule_on_worker(task, workerid);

    MIR_RECORDER_STATE_END(NULL, 0);
} /*}}}*/

static void mir_task_destroy(struct mir_task_t* task)
{ /*{{{*/
    // FIXME: Free the task!
    MIR_ASSERT(task != NULL);
} /*}}}*/

void mir_task_execute_prolog(struct mir_task_t* task)
{ /*{{{*/
    MIR_ASSERT(task != NULL);

    // Get this worker
    struct mir_worker_t* worker = mir_worker_get_context();
    MIR_ASSERT(worker != NULL);

    //MIR_LOG_INFO("worker %d task %" MIR_FORMSPEC_UL " start", worker->id, task->id.uid);

    if (runtime->enable_recorder == 1) {
        // Compose event metadata
        char event_meta_data_pre[MIR_RECORDER_EVENT_META_DATA_MAX_SIZE - 1] = { 0 };
        if (worker->current_task) {
            // Event is attributed to the current task
            char temp[MIR_LONG_NAME_LEN] = { 0 };
            sprintf(temp, "%" MIR_FORMSPEC_UL ",%s", worker->current_task->id.uid, worker->current_task->name);
            MIR_ASSERT(strlen(temp) < (MIR_RECORDER_EVENT_META_DATA_MAX_SIZE - 1));
            strcpy(event_meta_data_pre, temp);
        }
        else {
            sprintf(event_meta_data_pre, "0,in_task_execute_prolog");
        }
        // Record event
        MIR_RECORDER_EVENT(&event_meta_data_pre[0], MIR_RECORDER_EVENT_META_DATA_MAX_SIZE - 1);
        // Record state
        if (strcmp(task->metadata, "NA") == 0)
        {
            if (strcmp(task->name, MIR_IDLE_TASK_NAME) == 0) {
                MIR_RECORDER_STATE_BEGIN(MIR_STATE_TIMPLICIT);
            } else if (strcmp(task->name, "GOMP_parallel_task") == 0) {
                MIR_RECORDER_STATE_BEGIN(MIR_STATE_TOMP_PAR);
            } else if (strncmp(task->name, "GOMP_parallel_for", 17) == 0) {
                MIR_RECORDER_STATE_BEGIN(MIR_STATE_TOMP_PAR);
            } else {
                MIR_RECORDER_STATE_BEGIN(MIR_STATE_TEXEC);
            }
        } else {
            if (strcmp(task->metadata, "chunk_continuation") == 0 ||
                strcmp(task->metadata, "chunk_start") == 0 ) {
                MIR_RECORDER_STATE_BEGIN(MIR_STATE_TCHUNK_BOOK);
            }
            else {
                MIR_RECORDER_STATE_BEGIN(MIR_STATE_TCHUNK_ITER);
            }
        }
    }

    // Current task timing
    if (worker->current_task)
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
    if (runtime->enable_ofp_handshake == 1) {
        char buf[MIR_OFP_SHM_SIZE] = { 0 };
        sprintf(buf, "%" MIR_FORMSPEC_UL, task->id.uid);
        for (int i = 0; i < MIR_OFP_SHM_SIZE; i++)
            runtime->ofp_shm[i] = buf[i];
    }
} /*}}}*/

void mir_task_execute_epilog(struct mir_task_t* task)
{ /*{{{*/
    MIR_ASSERT(task != NULL);

    // Get this worker
    struct mir_worker_t* worker = mir_worker_get_context();
    MIR_ASSERT(worker != NULL);

    //MIR_LOG_INFO("worker %d task %" MIR_FORMSPEC_UL " end", worker->id, task->id.uid);

    // Record where executed
    task->cpu_id = worker->cpu_id;

    // Current task timing
    task->exec_end_instant = mir_get_cycles() - runtime->init_time;
    task->exec_cycles += (mir_get_cycles() - task->exec_resume_instant);

    // Record sync point
    // NOTE: All sibling tasks will have the same sync point
    task->sync_pass = task->twc->num_passes;

    // Add to task list
    if (runtime->enable_task_stats == 1)
        mir_worker_update_task_list(worker, task);

    // Restore task context of worker
    worker->current_task = task->predecessor;

    // Current task timing
    if (worker->current_task)
        worker->current_task->exec_resume_instant = mir_get_cycles();

    if (runtime->enable_recorder == 1) {
        // Record event and state
        char event_meta_data_post[MIR_RECORDER_EVENT_META_DATA_MAX_SIZE - 1] = { 0 };
        char temp[MIR_LONG_NAME_LEN] = { 0 };
        sprintf(temp, "%" MIR_FORMSPEC_UL ",%s", task->id.uid, task->name);
        MIR_ASSERT(strlen(temp) < (MIR_RECORDER_EVENT_META_DATA_MAX_SIZE - 1));
        strcpy(event_meta_data_post, temp);
        MIR_RECORDER_STATE_END(&event_meta_data_post[0], MIR_RECORDER_EVENT_META_DATA_MAX_SIZE - 1);
        MIR_RECORDER_EVENT(&event_meta_data_post[0], MIR_RECORDER_EVENT_META_DATA_MAX_SIZE - 1);
    }

    // Mark task as done
    task->done = 1;

    // Update task wait counter
    task->twc->count_per_worker[worker->id]++;

    // Signal
    T_DBG("Ex", task);
    __sync_synchronize();

    // FIXME Destroy task !
    // NOTE: Destroying task upsets task list structure
} /*}}}*/

void mir_task_execute(struct mir_task_t* task)
{ /*{{{*/
    MIR_ASSERT(task != NULL);

    // Start profiling and book-keeping for task
    mir_task_execute_prolog(task);

    MIR_CONTEXT_EXIT;

    // Execute task function
    task->func(task->data);

    MIR_CONTEXT_ENTER;

    // Stop profiling and book-keeping for task
    // We use worker->current_task instead of task since task can be chained with fake tasks.
    // An example of chaining is the execution of loop chunks as tasks.
    struct mir_worker_t* worker = mir_worker_get_context();
    MIR_ASSERT(worker != NULL);
    mir_task_execute_epilog(worker->current_task);

    // Debugging
    //MIR_LOG_INFO("Task %" MIR_FORMSPEC_UL " executed on worker %d\n", task->id.uid, worker->id);
} /*}}}*/

void mir_task_write_metadata(struct mir_task_t* task, const char* metadata)
{/*{{{*/
    MIR_ASSERT(task != NULL);
    MIR_ASSERT(metadata == NULL || strlen(metadata) < MIR_SHORT_NAME_LEN);
    strcpy(task->metadata, metadata ? metadata : "NA");
}/*}}}*/

#ifdef MIR_MEM_POL_ENABLE
static inline void mir_data_footprint_get_mem_node_dist(struct mir_mem_node_dist_t* dist, const struct mir_data_footprint_t* footprint)
{ /*{{{*/
    // Check
    MIR_ASSERT(dist != NULL);
    MIR_ASSERT(footprint != NULL);

    uint64_t row_sz = footprint->row_sz;
    if (row_sz > 1) {
        for (uint64_t brow = 0; brow <= footprint->end; brow++) {
            void* base = footprint->base + brow * row_sz * footprint->type;
            mir_mem_get_mem_node_dist(dist, base,
                (footprint->end - footprint->start + 1) * footprint->type,
                footprint->part_of);
            /*fprintf(stderr, "Footprint composed of these addresses:\n");*/
            /*fprintf(stderr, "%p[%lu-%lu]\n", base, footprint->start, footprint->end);*/
        }
    }
    else {
        mir_mem_get_mem_node_dist(dist, footprint->base,
            (footprint->end - footprint->start + 1) * footprint->type,
            footprint->part_of);
        /*fprintf(stderr, "Footprint composed of these addresses:\n");*/
        /*fprintf(stderr, "%p[%lu-%lu]\n", base, footprint->start, footprint->end);*/
    }
} /*}}}*/

// FIXME: Multi-tasking fucntion. Seperate!
struct mir_mem_node_dist_t* mir_task_get_mem_node_dist(struct mir_task_t* task, mir_data_access_t access)
{ /*{{{*/
    if (task->num_data_footprints == 0) {
        return NULL;
    }

    if (task->dist_by_access_type[access] == NULL) {
        // Allocate dist
        struct mir_mem_node_dist_t* dist = mir_mem_node_dist_create();
        task->dist_by_access_type[access] = dist;

        // Calculate dist
        for (int i = 0; i < task->num_data_footprints; i++)
            if (task->data_footprints[i].data_access == access)
                mir_data_footprint_get_mem_node_dist(dist, &task->data_footprints[i]);
    }

    return task->dist_by_access_type[access];
} /*}}}*/
#endif

void mir_task_wait_int(struct mir_twc_t* twc, int newval)
{ /*{{{*/
    MIR_RECORDER_STATE_BEGIN(MIR_STATE_TSYNC);

    struct mir_worker_t* worker = mir_worker_get_context();
    MIR_ASSERT(worker != NULL);

    // Prevent empty synchronizations
    // This upsets finding next forks in the task graph plotter
    if (!(twc->count > 0)) {
        // TODO: Report this!
        // MIR_ASSERT_STR(twc->count > 0, "An empty task synchronization occured. Such events upset task graph profiling.");
        goto exit;
    }

    // Wait and do useful work
    while (mir_twc_reduce(twc) != 1) {
        // __sync_synchronize();
        // Sync with or without backoff
        mir_worker_do_work(worker, MIR_WORKER_BACKOFF_DURING_SYNC);
    }

    // Record when passed and update num times passed
    // TODO: Should time update be locked?
    if (worker->current_task)
        twc->pass_time->time = elapsed_execution_time(worker->current_task);
    struct mir_time_list_t* tl = mir_malloc_int(sizeof(struct mir_time_list_t));
    MIR_CHECK_MEM(tl != NULL);
    tl->time = 0; // 0 => Not passed.
    tl->next = twc->pass_time;
    twc->pass_time = tl;
    __sync_fetch_and_add(&(twc->num_passes), 1);

    // Reset counts
    twc->count = newval;
    for (int i = 0; i < runtime->num_workers; i++)
        twc->count_per_worker[i] = 0;

exit:
    MIR_RECORDER_STATE_END(NULL, 0);

    return;
} /*}}}*/

void mir_task_wait()
{ /*{{{*/
    struct mir_worker_t* worker = pthread_getspecific(runtime->worker_index);
    struct mir_twc_t* twc;
    if (worker->current_task)
        twc = worker->current_task->ctwc;
    else
        twc = runtime->ctwc;

    mir_task_wait_int(twc, 0);

    return;
} /*}}}*/

void mir_task_stats_write_header_to_file(FILE* file)
{ /*{{{*/
    /* TODO:
     * - Rename create_instant to create_instant_rel_parent
     * - Rename wait_instants to wait_instants_rel_self
     * - Rename exec_end_instant to exec_end_instant_rel_runtime
     */
    fprintf(file, "task,parent,joins_at,cpu_id,child_number,num_children,exec_cycles,creation_cycles,overhead_cycles,queue_size,create_instant,exec_end_instant,tag,metadata,outline_function,wait_instants\n");
} /*}}}*/

void mir_task_stats_write_to_file(struct mir_task_list_t* list, FILE* file)
{ /*{{{*/
    struct mir_task_list_t* temp = list;
    while (temp != NULL) {
        mir_id_t task_parent;
        task_parent.uid = 0;
        if (temp->task->parent)
            task_parent.uid = temp->task->parent->id.uid;

        fprintf(file, "%" MIR_FORMSPEC_UL ",%" MIR_FORMSPEC_UL ",%lu,%u,%u,%u,%" MIR_FORMSPEC_UL ",%" MIR_FORMSPEC_UL ",%" MIR_FORMSPEC_UL ",%u,%" MIR_FORMSPEC_UL ",%" MIR_FORMSPEC_UL ",%s,%s,%p,[",
            temp->task->id.uid,
            task_parent.uid,
            temp->task->sync_pass,
            temp->task->cpu_id,
            temp->task->child_number,
            temp->task->num_children,
            temp->task->exec_cycles,
            temp->task->creation_cycles,
            temp->task->overhead_cycles,
            temp->task->queue_size_at_pop,
            temp->task->create_instant,
            temp->task->exec_end_instant,
            temp->task->name,
            temp->task->metadata,
            temp->task->func);

        struct mir_time_list_t* tl = temp->task->ctwc->pass_time;
        fprintf(file, "%" MIR_FORMSPEC_UL, tl->time);
        tl = tl->next;
        while (tl != NULL) {
            fprintf(file, ";%" MIR_FORMSPEC_UL, tl->time);
            tl = tl->next;
        };

        fprintf(file, "]\n");

        temp = temp->next;
    }
} /*}}}*/

void mir_task_list_destroy(struct mir_task_list_t* list)
{ /*{{{*/
    struct mir_task_list_t* temp = list;
    while (temp != NULL) {
        struct mir_task_list_t* next = temp->next;
        mir_free_int(temp, sizeof(struct mir_task_list_t));
        temp = next;
        // FIXME: Can also free the task and wait counter here!
    }
} /*}}}*/
