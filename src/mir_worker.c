#include "mir_worker.h"
#include "mir_task.h"
#include "mir_recorder.h"
#include "mir_sched_pol.h"
#include "mir_runtime.h"
#include "mir_memory.h"
#include "mir_debug.h"
#include "mir_lock.h"
#include "mir_defines.h"

#ifdef __tile__
#include <tmc/cpus.h>
#endif
#include <string.h>
#include <stdbool.h>

uint32_t g_worker_status_board = 0;
uint32_t g_num_tasks_waiting = 0;

extern uint32_t g_sig_worker_alive;
extern struct mir_runtime_t* runtime;

void* mir_worker_loop(void* arg)
{/*{{{*/
    struct mir_worker_t* worker = (struct mir_worker_t*) arg;

    // Initialize worker data
    mir_worker_local_init(worker);
    MIR_DEBUG(MIR_DEBUG_STR "Worker %d is initialized!\n", worker->id);

    // Signal runtime
    __sync_fetch_and_add(&g_sig_worker_alive, -1);

    // Now do useful work
    MIR_RECORDER_EVENT(NULL,0);
    MIR_RECORDER_STATE_BEGIN(MIR_STATE_TIDLE);
    while(1)
    {
        // Do work with backoff=true
        mir_worker_do_work(worker, true);

        // Check for runtime shutdown
        // __sync_synchronize();
        if(runtime->sig_dying == 1)
        {
            // Dump MIR_STATE_TIDLE state
            MIR_RECORDER_STATE_END(NULL, 0);
            MIR_RECORDER_EVENT(NULL,0);

            // Wait
            mir_lock_set(&worker->sig_die);
            //MIR_DEBUG(MIR_DEBUG_STR "Worker %d is dead!\n", worker->id);
            break;
        }
    }

    return NULL;
}/*}}}*/

void mir_worker_master_init(struct mir_worker_t* worker)
{/*{{{*/
    // Kill signal
    // Used during runtime system shutdown
    // When this is unset, the worker dies
    mir_lock_create(&worker->sig_die);
    mir_lock_set(&worker->sig_die);

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
    if (rval != 0)
        MIR_ABORT(MIR_ERROR_STR "Unable to create worker thread!\n");
}/*}}}*/

struct mir_worker_t* mir_worker_get_context()
{/*{{{*/
    return pthread_getspecific(runtime->worker_index);
}/*}}}*/

static int worker_get_cpu()
{/*{{{*/
    cpu_set_t set;
    CPU_ZERO(&set);

    int ret = sched_getaffinity(0, sizeof(cpu_set_t), &set);
    if(ret == -1)
        MIR_ABORT(MIR_ERROR_STR "Could not get cpu affinity of worker!\n");

    int cpu = 0;
    for(int i=0; i<CPU_SETSIZE; i++)
    {
        if(CPU_ISSET(i, &set))
        {
            cpu = i;
            break;
        }
    }

    return cpu;
}/*}}}*/

void mir_worker_local_init(struct mir_worker_t* worker)
{/*{{{*/
    // Set TLS
    int rval = pthread_setspecific(runtime->worker_index, (void*) worker);
    if (rval != 0)
        MIR_ABORT(MIR_ERROR_STR "Could not set TLS for worker %d!\n", worker->id);

    // Set the bias
    worker->bias = worker->id;

    // Set the backoff
    worker->backoff_us = MIR_WORKER_EXP_BOFF_RESET;

    // Bind the worker
    MIR_DEBUG(MIR_DEBUG_STR "Binding worker to core %d\n", worker->id);
#ifdef __tile__
    if (tmc_cpus_set_my_cpu(worker->id) != 0)
        MIR_ABORT(MIR_ERROR_STR "Unable to bind worker to core %d\n", worker->id);
#else
    cpu_set_t cpu_set;
    CPU_ZERO(&cpu_set);
    CPU_SET(worker->id, &cpu_set);
    sched_setaffinity((pid_t)0, sizeof(cpu_set), &cpu_set);
#endif

    // Create and reset worker status counters
    worker->status = (struct mir_worker_status_t*) mir_malloc_int (sizeof(struct mir_worker_status_t));
    if(worker->status == NULL)
        MIR_ABORT(MIR_ERROR_STR "Unable to create worker status!\n");
    mir_worker_status_init(worker->status);

    // Wait till bound
    while(worker->id != worker_get_cpu());

    // Create worker recorder
    worker->recorder = NULL;
    if(runtime->enable_recorder)
        worker->recorder = mir_recorder_create(worker->id);

    // Set current task
    worker->current_task = NULL;

    // For task graph generation
    worker->task_graph_node = NULL;
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

void mir_worker_do_work(struct mir_worker_t* worker, bool backoff)
{/*{{{*/
    // Try to find tasks to execute
    struct mir_task_t* task = NULL;
    bool work_available = runtime->sched_pol->pop(&task);
    if(work_available == 1) 
    {
        // Update busy counter
        __sync_fetch_and_add(&g_worker_status_board, 1);

        // Execute task
        mir_task_execute(task);

        // Update busy counter
        __sync_fetch_and_sub(&g_worker_status_board, 1);

        /*if(backoff)*/
        /*{*/
            // Update backoff
            mir_worker_backoff_reset(worker);
        /*}*/
    }
    else 
    { 
        // Do other useful things such as ...
        // Release independant tasks
        // For now we backoff
        if(backoff)
        {
            mir_worker_backoff(worker);
        }
    }
}/*}}}*/

//uint16_t mir_worker_get_id(struct mir_worker_t* worker)
/*{[>{{{<]*/
    /*return worker->id;*/
/*}[>}}}<]*/

void mir_worker_check_done()
{/*{{{*/
    while(1)
    {
        //mir_sleep_ms(300); // Butterfly effect!
        //__sync_synchronize();
        // Check if worker is free and no tasks are queued up
        //MIR_DEBUG(MIR_DEBUG_STR "Tasks waiting = %u Worker status %u \n", g_num_tasks_waiting, g_worker_status_board);
        if( g_num_tasks_waiting == 0 && g_worker_status_board == 0 )
            break;
    }
}/*}}}*/

void mir_worker_update_bias(struct mir_worker_t* worker)
{/*{{{*/
    worker->bias = (worker->bias + 1)%(runtime->num_workers);
}/*}}}*/

void mir_worker_status_init(struct mir_worker_status_t* status)
{/*{{{*/
    if(status)
    {
        // Get this worker
        struct mir_worker_t* worker = mir_worker_get_context();
        status->id = worker->id;
        status->num_tasks_created = 0;
        status->num_tasks_owned = 0;
        status->num_tasks_stolen = 0;
        status->num_tasks_inlined = 0;
        status->num_comm_tasks = 0;
        status->total_comm_cost = 0;
        status->lowest_comm_cost = -1; 
        status->highest_comm_cost = 0;
        if(0 == strcmp(runtime->sched_pol->name, "numa") && runtime->arch->diameter > 0)
        {
            status->num_comm_tasks_stolen_by_diameter = mir_malloc_int(sizeof(uint32_t) * runtime->arch->diameter);
            for(uint16_t i=0; i<runtime->arch->diameter; i++)
                status->num_comm_tasks_stolen_by_diameter[i] = 0;
        }
        else
        {
            status->num_comm_tasks_stolen_by_diameter = NULL;
        }
    }
}/*}}}*/

void mir_worker_status_destroy(struct mir_worker_status_t* status)
{/*{{{*/
    if(status && status->num_comm_tasks_stolen_by_diameter)
        mir_free_int(status->num_comm_tasks_stolen_by_diameter, sizeof(uint32_t) * runtime->arch->diameter);
}/*}}}*/

void mir_worker_status_update_comm_cost(struct mir_worker_status_t* status, unsigned long comm_cost)
{/*{{{*/
    status->num_comm_tasks++;
    status->total_comm_cost += comm_cost;
    if(status->lowest_comm_cost > comm_cost)
        status->lowest_comm_cost = comm_cost;
    if(status->highest_comm_cost < comm_cost)
        status->highest_comm_cost = comm_cost;
}/*}}}*/

void mir_worker_status_write_header_to_file(FILE* file)
{/*{{{*/
    fprintf(file, "worker,created,owned,stolen,inlined,comm_tasks,total_comm_cost,avg_comm_cost,lowest_comm_cost,highest_comm_cost,comm_tasks_stolen_by_diameter\n");
}/*}}}*/

void mir_worker_status_write_to_file(struct mir_worker_status_t* status, FILE* file)
{/*{{{*/
    if(status)
    {
        // Process
        // NOTE: This excludes inlined tasks
        unsigned long avg_comm_cost = status->total_comm_cost;
        if(status->num_comm_tasks > 1)
            avg_comm_cost /= status->num_comm_tasks;

        // Dump to file
        if(status->num_comm_tasks_stolen_by_diameter)
        {
            fprintf(file, "%d,%d,%d,%d,%d,%d,%lu,%lu,%lu,%lu",
                    status->id, 
                    status->num_tasks_created,
                    status->num_tasks_owned,
                    status->num_tasks_stolen,
                    status->num_tasks_inlined, 
                    status->num_comm_tasks,
                    status->total_comm_cost, 
                    avg_comm_cost,
                    status->lowest_comm_cost,
                    status->highest_comm_cost);
            fprintf(file, ",[");
            for(uint16_t i=0; i<runtime->arch->diameter; i++)
                fprintf(file, "%d-", status->num_comm_tasks_stolen_by_diameter[i]);
            fprintf(file, "]\n");

        }
        else
        {
            fprintf(file, "%d,%d,%d,%d,%d,%d,%lu,%lu,%lu,%lu,NA\n",
                    status->id, 
                    status->num_tasks_created,
                    status->num_tasks_owned,
                    status->num_tasks_stolen,
                    status->num_tasks_inlined, 
                    status->num_comm_tasks,
                    status->total_comm_cost, 
                    avg_comm_cost,
                    status->lowest_comm_cost,
                    status->highest_comm_cost);
        }
    }
}/*}}}*/

void mir_worker_update_task_graph(struct mir_worker_t* worker, struct mir_task_t* task)
{/*{{{*/
    if(runtime->enable_task_graph_gen == 1)
    {
        struct mir_task_graph_node_t* node = mir_malloc_int(sizeof(struct mir_task_graph_node_t));
        node->task = task;
        if(task->twc)
            node->pass_count = task->twc->num_passes;
        else
            node->pass_count = 0;
        node->next = worker->task_graph_node;
        worker->task_graph_node = node;
    }
}/*}}}*/

void mir_task_graph_write_header_to_file(FILE* file)
{/*{{{*/
    fprintf(file, "task,parent,join_node,join_node_parent,join_node_pass_count,execution_start_time\n");
}/*}}}*/

void mir_task_graph_write_to_file(struct mir_task_graph_node_t* node, FILE* file)
{/*{{{*/
    struct mir_task_graph_node_t* temp = node;
    while(temp != NULL)
    {
        mir_id_t task_parent, twc_parent;
        task_parent.uid = 0;
        twc_parent.uid = 0;
        if(temp->task->parent)
            task_parent.uid = temp->task->parent->id.uid;
        if(temp->task->twc)
            if(temp->task->twc->parent)
                twc_parent.uid = temp->task->twc->parent->id.uid;

        fprintf(file, "%" MIR_FORMSPEC_UL ",%" MIR_FORMSPEC_UL ",%" MIR_FORMSPEC_UL ",%" MIR_FORMSPEC_UL ",%lu" ",%" MIR_FORMSPEC_UL "\n", 
                temp->task->id.uid, 
                task_parent.uid,
                (temp->task->twc == NULL)? 0 : temp->task->twc->id.uid,
                twc_parent.uid,
                temp->pass_count,
                temp->task->execution_start_time);

        temp = temp->next;
    }
}/*}}}*/

void mir_task_graph_destroy(struct mir_task_graph_node_t* node)
{/*{{{*/
    struct mir_task_graph_node_t* this = node;
    while(this != NULL)
    {
        struct mir_task_graph_node_t* next = this->next;
        mir_free_int(this, sizeof(struct mir_task_graph_node_t));
        this = next;
        // FIXME: Can also free the task and wait counter here!
    }
}/*}}}*/
