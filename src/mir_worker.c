#include "mir_defines.h"
#include "mir_lock.h"
#include "mir_memory.h"
#include "mir_recorder.h"
#include "mir_runtime.h"
#include "mir_task.h"
#include "mir_utils.h"
#include "mir_worker.h"
#include "arch/mir_arch.h"
#include "scheduling/mir_sched_pol.h"

#ifdef __tile__
#include <tmc/cpus.h>
#endif
#include <string.h>


// FIXME: Make these per-worker
// PJ says kill the thread upon exit
uint32_t g_worker_status_board = 0;
uint32_t g_num_tasks_waiting = 0;

extern uint32_t g_sig_worker_alive;
extern struct mir_runtime_t* runtime;

static void* mir_worker_loop(void* arg)
{/*{{{*/
    struct mir_worker_t* worker = (struct mir_worker_t*) arg;
    MIR_ASSERT(worker != NULL);

    // Initialize worker data
    mir_worker_local_init(worker);
    MIR_DEBUG(MIR_DEBUG_STR "Worker %d is initialized!\n", worker->id);

    // Signal runtime
    __sync_fetch_and_add(&g_sig_worker_alive, -1);
    MIR_ASSERT(g_sig_worker_alive < runtime->num_workers);

    // Record state and event
    MIR_RECORDER_EVENT(NULL,0);
    MIR_RECORDER_STATE_BEGIN(MIR_STATE_TIDLE);

    // Now do useful work
    while(1)
    {
        // Do work with backoff=1
        mir_worker_do_work(worker, 1);

        // Check for runtime shutdown
        // __sync_synchronize();
        if(worker->sig_dying == 1)
        {
            // Dump MIR_STATE_TIDLE state
            MIR_RECORDER_STATE_END(NULL, 0);
            MIR_RECORDER_EVENT(NULL,0);

            // Wait
            mir_lock_set(&worker->sig_die);
            MIR_ASSERT(g_sig_worker_alive < runtime->num_workers);
            __sync_fetch_and_add(&g_sig_worker_alive, -1);
            MIR_DEBUG(MIR_DEBUG_STR "Worker %d is dead!\n", worker->id);
            break;
        }
    }

    return NULL;
}/*}}}*/

void mir_worker_master_init(struct mir_worker_t* worker)
{/*{{{*/
    MIR_ASSERT(worker != NULL);
    // Kill signal
    // Used during runtime system shutdown
    // When this is unset, the worker dies
    mir_lock_create(&worker->sig_die);
    mir_lock_set(&worker->sig_die);
    worker->sig_dying = 0;

    // Worker thread attributes
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    // TODO: Create stack on heap
    // TODO: Allocate stack for local (NUCA and NUMA) access
    // Thread stack size
    size_t stacksize;
    pthread_attr_getstacksize (&attr, &stacksize);
    stacksize *= MIR_WORKER_STACK_SIZE_MULTIPLIER;
    pthread_attr_setstacksize (&attr, stacksize);

    // Create worker thread
    int rval = pthread_create(&(worker->pthread), &attr, mir_worker_loop, (void*)worker);
    MIR_ASSERT(rval == 0);
}/*}}}*/

struct mir_worker_t* mir_worker_get_context()
{/*{{{*/
    struct mir_worker_t* worker = pthread_getspecific(runtime->worker_index);
    MIR_ASSERT(worker != NULL);
    return worker;
}/*}}}*/

static int worker_get_cpu_affinity()
{/*{{{*/
    cpu_set_t set;
    CPU_ZERO(&set);

    int ret = sched_getaffinity(0, sizeof(cpu_set_t), &set);
    MIR_ASSERT(ret != -1);

    int cpu = -1;
    for(int i=0; i<CPU_SETSIZE; i++)
    {
        if(CPU_ISSET(i, &set))
        {
            cpu = i;
            break;
        }
    }
    MIR_ASSERT(cpu != -1);

    return cpu;
}/*}}}*/

void mir_worker_local_init(struct mir_worker_t* worker)
{/*{{{*/
    MIR_ASSERT(worker != NULL);

    // Set TLS
    int rval = pthread_setspecific(runtime->worker_index, (void*) worker);
    MIR_ASSERT(rval == 0);

    // Set the bias
    worker->bias = worker->id;

    // Set the backoff
    worker->backoff_us = MIR_WORKER_EXP_BOFF_RESET;

    // Bind the worker
    MIR_DEBUG(MIR_DEBUG_STR "Binding worker %d to cpu %d\n", worker->id, worker->cpu_id);
#ifdef __tile__
    int ret = tmc_cpus_set_my_cpu(worker->cpu_id);
    MIR_ASSERT(ret == 0);
#else
    cpu_set_t cpu_set;
    CPU_ZERO(&cpu_set);
    int sys_cpu = runtime->arch->sys_cpu_of(worker->cpu_id);
    CPU_SET(sys_cpu, &cpu_set);
    sched_setaffinity(0, sizeof(cpu_set), &cpu_set);
#endif

    // Create and reset worker statistics counters
    worker->statistics = NULL;
    if(runtime->enable_worker_stats == 1)
    {
        worker->statistics = mir_malloc_int (sizeof(struct mir_worker_statistics_t));
        MIR_ASSERT(worker->statistics != NULL);
        mir_worker_statistics_init(worker->statistics);
    }

    // Wait till bound
#ifdef __tile__
    // TODO
#else
    while(sys_cpu != worker_get_cpu_affinity());
#endif

    // Create worker recorder
    worker->recorder = NULL;
    if(runtime->enable_recorder == 1)
        worker->recorder = mir_recorder_create(worker->id);

    // Set current task
    worker->current_task = NULL;

    // For task statistics collection 
    worker->task_list = NULL;

    // Create private task queue
    worker->private_queue = mir_queue_create(runtime->sched_pol->queue_capacity);
    MIR_ASSERT(worker->private_queue != NULL);
}/*}}}*/

static inline void mir_worker_backoff_reset(struct mir_worker_t* worker)
{/*{{{*/
    worker->backoff_us = MIR_WORKER_EXP_BOFF_RESET;
}/*}}}*/

static inline void mir_worker_backoff(struct mir_worker_t* worker)
{/*{{{*/
    mir_sleep_us(worker->backoff_us);
    if(worker->backoff_us < MIR_WORKER_EXP_BOFF_ROOF)
        worker->backoff_us *= MIR_WORKER_EXP_BOFF_SCALE;
}/*}}}*/

void mir_worker_push(struct mir_worker_t* worker, struct mir_task_t* task)
{/*{{{*/
    MIR_ASSERT(worker != NULL);
    MIR_ASSERT(task != NULL);

    if( 0 == mir_queue_push(worker->private_queue, (void*) task) )
        MIR_ABORT(MIR_ERROR_STR "Cannot enque task into private queue. Increase queue capacity using MIR_CONF.\n");

    __sync_fetch_and_add(&g_num_tasks_waiting, 1);
    // Update stats
    if(runtime->enable_worker_stats == 1)
        worker->statistics->num_tasks_created++;
}/*}}}*/

static inline int mir_worker_pop(struct mir_worker_t* worker, struct mir_task_t** task)
{/*{{{*/
    MIR_ASSERT(worker != NULL);
    MIR_ASSERT(task != NULL);

    int found = 0;
    struct mir_queue_t* queue = worker->private_queue;
    MIR_ASSERT(worker->private_queue != NULL);
    if(mir_queue_size(queue) > 0)
    {
        mir_queue_pop(queue, (void**)&(*task));
        // There is no other popper. This must succeed.
        MIR_ASSERT(*task != NULL);
        __sync_fetch_and_sub(&g_num_tasks_waiting, 1);
        T_DBG("Dq", *task);

        found = 1;
    }

    return found;
}/*}}}*/

void mir_worker_do_work(struct mir_worker_t* worker, int backoff)
{/*{{{*/
    MIR_ASSERT(worker != NULL);

    // Try to find tasks to execute
    struct mir_task_t* task = NULL;
    int work_available = 0;

    // Overhead measurement 
    uint64_t start_instant = mir_get_cycles();

    work_available = mir_worker_pop(worker, &task);

    // Overhead measurement 
    if(worker->current_task) worker->current_task->overhead_cycles += (mir_get_cycles() - start_instant); 

    if(work_available == 1) 
    {
        // Update busy counter
        __sync_fetch_and_add(&g_worker_status_board, 1);

        // Execute task
        mir_task_execute(task);

        // Update busy counter
        __sync_fetch_and_sub(&g_worker_status_board, 1);

        // Update backoff
        mir_worker_backoff_reset(worker);

        return;
    }
    
    // Overhead measurement 
    start_instant = mir_get_cycles();

    work_available = runtime->sched_pol->pop(&task);

    // Overhead measurement 
    if(worker->current_task) worker->current_task->overhead_cycles += (mir_get_cycles() - start_instant); 

    if(work_available == 1) 
    {
        // Update busy counter
        __sync_fetch_and_add(&g_worker_status_board, 1);

        // Execute task
        mir_task_execute(task);

        // Update busy counter
        __sync_fetch_and_sub(&g_worker_status_board, 1);

        // Update backoff
        mir_worker_backoff_reset(worker);

        return;
    }
    
    // Overhead measurement 
    start_instant = mir_get_cycles();

    // Do other useful things such as ...
    // Release independant tasks
    // For now we backoff
    if(backoff)
    {
        mir_worker_backoff(worker);
    }
    
    // Overhead measurement 
    if(worker->current_task) worker->current_task->overhead_cycles += (mir_get_cycles() - start_instant); 
}/*}}}*/

void mir_worker_check_done()
{/*{{{*/
    while(1)
    {
        //mir_sleep_ms(300); // Butterfly effect!
        __sync_synchronize();
        // Check if worker is free and no tasks are queued up
        //MIR_DEBUG(MIR_DEBUG_STR "Tasks waiting = %u Worker status %u \n", g_num_tasks_waiting, g_worker_status_board);
        if( g_num_tasks_waiting == 0 && g_worker_status_board == 0 )
            if( g_num_tasks_waiting == 0 && g_worker_status_board == 0 )
                    // Rechecking to ensure. Hope compiler does not optimize this away!
                    break;
    }
    MIR_ASSERT(g_num_tasks_waiting == 0);
    MIR_ASSERT(g_worker_status_board == 0);
}/*}}}*/

void mir_worker_update_bias(struct mir_worker_t* worker)
{/*{{{*/
    MIR_ASSERT(worker != NULL);
    worker->bias = (worker->bias + 1)%(runtime->num_workers);
}/*}}}*/

void mir_worker_statistics_init(struct mir_worker_statistics_t* statistics)
{/*{{{*/
    MIR_ASSERT(statistics != NULL);

    // Get this worker
    struct mir_worker_t* worker = mir_worker_get_context();
    MIR_ASSERT(worker != NULL);

    // Set statistics
    statistics->id = worker->id;
    statistics->num_tasks_created = 0;
    statistics->num_tasks_owned = 0;
    statistics->num_tasks_stolen = 0;
    statistics->num_tasks_inlined = 0;
    statistics->num_comm_tasks = 0;
    statistics->total_comm_cost = 0;
    statistics->lowest_comm_cost = -1; 
    statistics->highest_comm_cost = 0;
#ifdef MIR_MEM_POL_ENABLE
        if(0 == strcmp(runtime->sched_pol->name, "numa") && runtime->arch->diameter > 0)
        {
            statistics->num_comm_tasks_stolen_by_diameter = mir_malloc_int(sizeof(uint32_t) * runtime->arch->diameter);
            MIR_ASSERT(statistics->num_comm_tasks_stolen_by_diameter != NULL);
            for(uint16_t i=0; i<runtime->arch->diameter; i++)
                statistics->num_comm_tasks_stolen_by_diameter[i] = 0;
        }
        else
        {
            statistics->num_comm_tasks_stolen_by_diameter = NULL;
        }
#else
        statistics->num_comm_tasks_stolen_by_diameter = NULL;
#endif
}/*}}}*/

void mir_worker_statistics_destroy(struct mir_worker_statistics_t* statistics)
{/*{{{*/
    MIR_ASSERT(statistics != NULL);
    if(statistics->num_comm_tasks_stolen_by_diameter)
        mir_free_int(statistics->num_comm_tasks_stolen_by_diameter, sizeof(uint32_t) * runtime->arch->diameter);
}/*}}}*/

void mir_worker_statistics_update_comm_cost(struct mir_worker_statistics_t* statistics, unsigned long comm_cost)
{/*{{{*/
    MIR_ASSERT(statistics != NULL);
    statistics->num_comm_tasks++;
    statistics->total_comm_cost += comm_cost;
    if(statistics->lowest_comm_cost > comm_cost)
        statistics->lowest_comm_cost = comm_cost;
    if(statistics->highest_comm_cost < comm_cost)
        statistics->highest_comm_cost = comm_cost;
}/*}}}*/

void mir_worker_statistics_write_header_to_file(FILE* file)
{/*{{{*/
    fprintf(file, "worker,created,owned,stolen,inlined,comm_tasks,total_comm_cost,avg_comm_cost,lowest_comm_cost,highest_comm_cost,comm_tasks_stolen_by_diameter\n");
}/*}}}*/

void mir_worker_statistics_write_to_file(const struct mir_worker_statistics_t* statistics, FILE* file)
{/*{{{*/
    MIR_ASSERT(file != NULL);
    MIR_ASSERT(statistics != NULL);

    // Pre-process
    unsigned long avg_comm_cost = statistics->total_comm_cost;
    if(statistics->num_comm_tasks > 1)
        // NOTE: We exclude inlined tasks
        avg_comm_cost /= statistics->num_comm_tasks;

    // Dump to file
    if(statistics->num_comm_tasks_stolen_by_diameter)
    {
        fprintf(file, "%d,%d,%d,%d,%d,%d,%lu,%lu,%lu,%lu",
                statistics->id, 
                statistics->num_tasks_created,
                statistics->num_tasks_owned,
                statistics->num_tasks_stolen,
                statistics->num_tasks_inlined, 
                statistics->num_comm_tasks,
                statistics->total_comm_cost, 
                avg_comm_cost,
                statistics->lowest_comm_cost,
                statistics->highest_comm_cost);
        fprintf(file, ",[");
        for(uint16_t i=0; i<runtime->arch->diameter; i++)
            fprintf(file, "%d-", statistics->num_comm_tasks_stolen_by_diameter[i]);
        fprintf(file, "]\n");
    }
    else
    {
        fprintf(file, "%d,%d,%d,%d,%d,%d,%lu,%lu,%lu,%lu,NA\n",
                statistics->id, 
                statistics->num_tasks_created,
                statistics->num_tasks_owned,
                statistics->num_tasks_stolen,
                statistics->num_tasks_inlined, 
                statistics->num_comm_tasks,
                statistics->total_comm_cost, 
                avg_comm_cost,
                statistics->lowest_comm_cost,
                statistics->highest_comm_cost);
    }
}/*}}}*/

void mir_worker_update_task_list(struct mir_worker_t* worker, struct mir_task_t* task)
{/*{{{*/
    MIR_ASSERT(worker != NULL);
    MIR_ASSERT(task != NULL);

    struct mir_task_list_t* list = mir_malloc_int(sizeof(struct mir_task_list_t));
    MIR_ASSERT(list != NULL);
    list->task = task;
    list->next = worker->task_list;
    worker->task_list = list;
}/*}}}*/

