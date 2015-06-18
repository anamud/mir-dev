#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <sys/shm.h>

#include "mir_runtime.h"
#include "mir_defines.h"
#include "mir_lock.h"
#include "mir_memory.h"
#include "mir_types.h"
#include "mir_recorder.h"
#include "mir_worker.h"
#include "scheduling/mir_sched_pol.h"
#include "arch/mir_arch.h"
#include "mir_utils.h"
#include "mir_omp_int.h"

#ifdef MIR_MEM_POL_ENABLE
#include "mir_mem_pol.h"
#endif

#ifdef MIR_RECORDER_USE_HW_PERF_COUNTERS
#ifndef __tile__
#include "papi.h"
#endif
#endif

// The global runtime object
struct mir_runtime_t* runtime = NULL;

// FIXME: Make this per-worker
uint32_t g_sig_worker_alive = 0;

static void mir_preconfig_init()
{
    /*{{{*/
    MIR_DEBUG(MIR_DEBUG_STR "Starting initialization ...\n");

    // Initialization control
    runtime->init_count = 1;
    runtime->destroyed = 0;

    // Arch
    runtime->arch = mir_arch_create_by_query();
    MIR_ASSERT(runtime->arch != NULL);
    MIR_DEBUG(MIR_DEBUG_STR "Architecture set to %s\n", runtime->arch->name);

    // Scheduling policy
    runtime->sched_pol = mir_sched_pol_get_by_name(MIR_SCHED_POL_DEFAULT);
    MIR_ASSERT(runtime->sched_pol != NULL);

#ifdef MIR_MEM_POL_ENABLE
    // Memory allocation policy
    mir_mem_pol_create();
#endif

    // Workers
    runtime->num_workers = runtime->arch->num_cores;
    runtime->worker_cpu_map = mir_malloc_int(sizeof(uint16_t) * runtime->arch->num_cores);
    MIR_ASSERT(runtime->worker_cpu_map != NULL);
    for (int i = 0; i < runtime->num_workers; i++)
        runtime->worker_cpu_map[i] = (uint16_t) i;
    int rval = pthread_key_create(&runtime->worker_index, NULL);
    MIR_ASSERT(rval == 0);

    // OpenMP support
    // This is the unnamed critical section lock
    mir_lock_create(&runtime->omp_critsec_lock);
    runtime->omp_for_schedule = OFS_STATIC;
    runtime->omp_for_chunk_size = 0;
    GOMP_parse_schedule();

    // Flags
    runtime->sig_dying = 0;
    runtime->enable_worker_stats = 0;
    runtime->enable_task_stats = 0;
    runtime->enable_recorder = 0;
    runtime->enable_ofp_handshake = 0;
    runtime->task_inlining_limit = MIR_INLINE_TASK_DURING_CREATION;
}/*}}}*/

static void mir_postconfig_init()
{
    /*{{{*/
    // Init time
    runtime->init_time = mir_get_cycles();

    // Scheduling policy
    runtime->sched_pol->create();
    MIR_DEBUG(MIR_DEBUG_STR "Task scheduling policy set to %s\n", runtime->sched_pol->name);

    // Enable communication between outline function profiler and MIR
    if (runtime->enable_ofp_handshake == 1)
    {
        /*{{{*/
        // Create shared memory
        runtime->ofp_shmid = shmget((int)(MIR_OFP_SHM_KEY), MIR_OFP_SHM_SIZE, IPC_CREAT | 0666);
        if (runtime->ofp_shmid < 0) MIR_ABORT(MIR_ERROR_STR "shmget failed [%d]!\n", runtime->ofp_shmid);

        // Attach
        runtime->ofp_shm = shmat(runtime->ofp_shmid, NULL, 0);
        MIR_ASSERT(runtime->ofp_shm != NULL);
    }/*}}}*/
    else
    {
        /*{{{*/
        runtime->ofp_shmid = -1;
        runtime->ofp_shm = NULL;
    }/*}}}*/

    // Recorder
    if (runtime->enable_recorder == 1)
    {
        /*{{{*/
#ifdef MIR_RECORDER_USE_HW_PERF_COUNTERS
#ifndef __tile__
        // PAPI
        int retval = PAPI_library_init( PAPI_VER_CURRENT );
        if ( retval != PAPI_VER_CURRENT )
            MIR_ABORT(MIR_ERROR_STR "PAPI_library_init failed [%d != %d]!\n", retval, PAPI_VER_CURRENT);
        retval = PAPI_thread_init(pthread_self);
        if ( retval != PAPI_OK )
            MIR_ABORT(MIR_ERROR_STR "PAPI_thread_init failed [%d]!\n", retval);
#endif
#endif
    }/*}}}*/

    // Workers
    __sync_fetch_and_add(&g_sig_worker_alive, runtime->num_workers - 1);
#ifdef __tile__
    __asm__ __volatile__ ("mf;"::);
#else
    __sync_synchronize();
#endif
#ifdef MIR_WORKER_EXPLICIT_BIND
    const char* worker_cpu_map_str = getenv("MIR_WORKER_CPU_MAP");
    if (worker_cpu_map_str)
        if (strlen(worker_cpu_map_str) > 0)
        {
            MIR_DEBUG(MIR_DEBUG_STR "Reading worker to cpu map ...\n");

            // Copy to buf
            char str[MIR_LONG_NAME_LEN];
            strcpy(str, worker_cpu_map_str);

            // Parse for cpu ids
            uint16_t tok_cnt = 0;
            char* tok = strtok (str, ",");
            while (tok != NULL && tok_cnt < runtime->num_workers)
            {
                int id;
                sscanf (tok, "%d", &id);
                //MIR_DEBUG(MIR_DEBUG_STR "Read token %d ...\n", id);
                runtime->worker_cpu_map[tok_cnt] = (uint16_t) id;
                tok_cnt++;
                tok = strtok (NULL, ",");
            }
            MIR_ASSERT(tok_cnt == runtime->num_workers);
        }
#endif
    for (uint32_t i = 0; i < runtime->num_workers; i++)
    {
        // FIXME: Master thread stack size, how to set?
        struct mir_worker_t* worker = &runtime->workers[i];
        worker->id = i;
        worker->cpu_id = runtime->worker_cpu_map[i];
        if (worker->id != 0)
            mir_worker_master_init(worker);
        if (worker->id == 0)
            mir_worker_local_init(worker);
    }

    // Wait for workers to signal alive
    MIR_ASSERT(g_sig_worker_alive < runtime->num_workers);
wait_alive:
    if (g_sig_worker_alive == 0) goto alive;
    __sync_synchronize();
    goto wait_alive;
alive:

    // Global taskwait counter
    runtime->ctwc = mir_twc_create();
    runtime->num_children_tasks = 0;

#ifdef MIR_MEM_POL_ENABLE
    // Memory allocation policy
    // Init memory distributer only after workers are bound.
    // Node restrictions can then be correctly inferred.
    mir_mem_pol_init();
#endif

    MIR_DEBUG(MIR_DEBUG_STR "Initialization complete!\n");
}/*}}}*/

static inline void print_help()
{
    /*{{{*/
    // Here all configuration components
    // ... should define the intention
    // ... of their config symbols

    MIR_INFORM(MIR_INFORM_STR "Valid options in MIR_CONF environment variable ...\n"
               "-h print this help message\n"
               "-w=<int> number of workers\n"
               "-s=<str> task scheduling policy. Choose among central, central-stack, ws, ws-de and numa.\n"
               "-r enable worker recorder\n"
               "-x=<int> task inlining limit based on num tasks per worker.\n"
               "-i collect worker statistics\n"
               "-l=<int> worker stack size in MB\n"
               "-q=<int> task queue capacity\n"
               "-m=<str> memory allocation policy. Choose among coarse, fine and system.\n"
               "-y=<csv> schedule policy specific parameters. Policy numa: data size in bytes below which task is dealt to worker's private queue.\n"
               "-g collect task statistics\n"
               "-p enable handshake with Outline Function Profiler [Note: Supported only for a single worker!]\n"
              );
}/*}}}*/

static void mir_config()
{
    /*{{{*/
    // Get MIR_CONF environment string
    const char* conf_str = getenv("MIR_CONF");
    if (!conf_str)
        return;

    if (strlen(conf_str) == 0)
        return;

    // Copy to buffer
    char str[MIR_LONG_NAME_LEN];
    strcpy(str, getenv("MIR_CONF"));

    // Parse tokens
    char* tok = strtok(str, " ");
    while (tok)
    {
        if (tok[0] == '-')
        {
            char c = tok[1];
            switch (c)
            {
            /*{{{*/
            case 'h':
                print_help();
                break;
            case 'w':
                if (tok[2] == '=')
                {
                    char* s = tok + 3;
                    runtime->num_workers = atoi(s);
                    if (runtime->num_workers > runtime->arch->num_cores)
                        MIR_ABORT(MIR_ERROR_STR "Cannot configure more workers (%d) than number of cores (%d) !\n", runtime->num_workers, runtime->arch->num_cores);
                }
                else
                {
                    MIR_ABORT(MIR_ERROR_STR "Incorrect MIR_CONF parameter [%c]\n", c);
                }
                break;
            case 's':
                if (tok[2] == '=')
                {
                    char* s = tok + 3;
                    runtime->sched_pol = mir_sched_pol_get_by_name(s);
                    if (runtime->sched_pol == NULL)
                        MIR_ABORT(MIR_ERROR_STR "Cannot select %s scheduling policy!\n", s);
                }
                else
                {
                    MIR_ABORT(MIR_ERROR_STR "Incorrect MIR_CONF parameter [%c]\n", c);
                }
                break;
            case 'g':
                runtime->enable_task_stats = 1;
                MIR_DEBUG(MIR_DEBUG_STR "Task statistics collection is enabled!\n");
                break;
            case 'r':
                runtime->enable_recorder = 1;
                MIR_DEBUG(MIR_DEBUG_STR "Recorder is enabled!\n");
                break;
            case 'i':
                runtime->enable_worker_stats = 1;
                MIR_DEBUG(MIR_DEBUG_STR "Worker statistics collection is enabled!\n");
                break;
            case 'x':
                if (tok[2] == '=')
                {
                    char* s = tok + 3;
                    runtime->task_inlining_limit = atoi(s);
                    MIR_DEBUG(MIR_DEBUG_STR "Task inlining limit set to %u\n", runtime->task_inlining_limit);
                }
                else
                {
                    MIR_ABORT(MIR_ERROR_STR "Incorrect MIR_CONF parameter [%c]\n", c);
                }
                break;
            case 'l':
                if (tok[2] == '=')
                {
                    char* s = tok + 3;
                    int ps_sz = atoi(s) * 1024 * 1024;
                    MIR_ASSERT(ps_sz > 0);
                    MIR_ASSERT(0 == mir_pstack_set_size(ps_sz));
                    MIR_DEBUG(MIR_DEBUG_STR "Process stack size set to %d bytes\n", ps_sz);
                }
                else
                {
                    MIR_ABORT(MIR_ERROR_STR "Incorrect MIR_CONF parameter [%c]\n", c);
                }
                break;
            case 'p':
                if (runtime->num_workers == 1)
                {
                    runtime->enable_ofp_handshake = 1;
                    MIR_DEBUG(MIR_DEBUG_STR "OFP handshake mode is enabled!\n");
                }
                else
                {
                    MIR_ABORT(MIR_ERROR_STR "Number of workers = %d != 1. Cannot enable OFP handshake mode!\n", runtime->num_workers);
                }
                break;
            default:
                break;
            }/*}}}*/
        }
        tok = strtok(NULL, " ");
    }

    // Pass token string to other configurable components
#ifdef MIR_MEM_POL_ENABLE
    mir_mem_pol_config(conf_str);
#endif
    runtime->sched_pol->config(conf_str);
    runtime->arch->config(conf_str);
}/*}}}*/

void mir_create()
{
    /*{{{*/
    // Create only if first call
    if (runtime != NULL)
    {
        MIR_ASSERT(runtime->destroyed == 0);
        __sync_fetch_and_add(&(runtime->init_count), 1);
        return;
    }

    // Create the global runtime
    MIR_ASSERT(runtime == NULL);
    runtime = mir_malloc_int(sizeof(struct mir_runtime_t));
    if ( NULL == runtime )
        MIR_ABORT(MIR_ERROR_STR "Unable to create the runtime!\n");

    // Set defaults and other stuff
    mir_preconfig_init();

    // Set configurable parameters from command line
    mir_config();

    // Initialize
    mir_postconfig_init();

    atexit(mir_destroy);

    // Set a marking event
    MIR_RECORDER_EVENT(NULL, 0);
}/*}}}*/

/**
    Reduces the nesting level counter but leaves the RTS mostly alive.

    Call @mir_destroy@ for proper destruction.
*/
void mir_soft_destroy()
{
    /*{{{*/
    MIR_ASSERT(runtime->init_count > 0);
    __sync_fetch_and_sub(&(runtime->init_count), 1);
}/*}}}*/

void mir_destroy()
{
    /*{{{*/
    // Destroy only once. Multiple calls happen if the user inserts 
    // explicit calls to mir_destroy() in the program.
    //MIR_ASSERT(runtime->destroyed == 0);
    if(runtime->destroyed == 1)
        return;

    // Destory only if corresponding to first call to mir_create
    __sync_fetch_and_sub(&(runtime->init_count), 1);
    if (runtime->init_count <= 0)
        runtime->destroyed = 1;
    else
        return;

    // Set a marking event
    MIR_RECORDER_EVENT(NULL, 0);

    MIR_DEBUG(MIR_DEBUG_STR "Shutting down ...\n");

    // Check if workers are free
    MIR_DEBUG(MIR_DEBUG_STR "Checking if workers are done ...\n");
    //MIR_INFORM(MIR_INFORM_STR "Checking if workers are done ...\n");
    mir_worker_check_done();

    // Announce destruction
    runtime->sig_dying = 1;

    // Freeze workers
    for (int i = 0; i < runtime->num_workers; i++)
        runtime->workers[i].sig_dying = 1;
    __sync_synchronize();
    MIR_DEBUG(MIR_DEBUG_STR "Workers are done. Sent die signal.\n");
    //MIR_INFORM(MIR_INFORM_STR "Workers are done. Sent die signal.\n");

    // Shutdown recorders
    if (runtime->enable_recorder == 1)
    {
        /*{{{*/
        MIR_DEBUG(MIR_DEBUG_STR "Shutting down recorders ...\n");
        for (int i = 0; i < runtime->num_workers; i++)
        {
            mir_recorder_write_to_file(runtime->workers[i].recorder);
            mir_recorder_destroy(runtime->workers[i].recorder);
        }
    }/*}}}*/

    // Dump worker statistics
    if (runtime->enable_worker_stats == 1)
    {
        /*{{{*/
        MIR_DEBUG(MIR_DEBUG_STR "Dumping worker stats ...\n");
        // Open stats file
        FILE* stats_file = NULL;
        stats_file = fopen(MIR_WORKER_STATS_FILE_NAME, "w");
        if (!stats_file)
            MIR_ABORT(MIR_ERROR_STR "Cannot open worker stats file %s for writing!\n", MIR_WORKER_STATS_FILE_NAME);

        // Write header
        mir_worker_statistics_write_header_to_file(stats_file);
        // Write all worker statistics counters to stats file
        for (int i = 0; i < runtime->num_workers; i++)
        {
            struct mir_worker_statistics_t* statistics = runtime->workers[i].statistics;
            mir_worker_statistics_write_to_file(statistics, stats_file);
            // FIXME: Maybe this should be done by the worker
            mir_worker_statistics_destroy(statistics);
        }

        // Close stats file
        fclose(stats_file);
    }/*}}}*/

    // Dump task statistics
    if (runtime->enable_task_stats == 1)
    {
        /*{{{*/
        MIR_DEBUG(MIR_DEBUG_STR "Dumping task statistics ...\n");
        // Open stats file
        FILE* task_statistics_file = NULL;
        task_statistics_file = fopen(MIR_TASK_STATS_FILE_NAME, "w");
        if (!task_statistics_file)
            MIR_ABORT(MIR_ERROR_STR "Cannot open task statistics file %s for writing!\n", MIR_TASK_STATS_FILE_NAME);

        // Write header
        mir_task_stats_write_header_to_file(task_statistics_file);
        // Write per-worker task statistics to file
        for (int i = 0; i < runtime->num_workers; i++)
        {
            struct mir_task_list_t* list = runtime->workers[i].task_list;
            mir_task_stats_write_to_file(list, task_statistics_file);
        }

        // Close task_statistics file
        fclose(task_statistics_file);
        
        // Open events file
        FILE* task_events_file = NULL;
        task_events_file = fopen(MIR_TASK_EVENTS_FILE_NAME, "w");
        if (!task_events_file)
            MIR_ABORT(MIR_ERROR_STR "Cannot open task events file %s for writing!\n", MIR_TASK_EVENTS_FILE_NAME);

        // Write header
        mir_task_events_write_header_to_file(task_events_file);
        // Write per-worker task events to file
        for (int i = 0; i < runtime->num_workers; i++)
        {
            struct mir_task_list_t* list = runtime->workers[i].task_list;
            mir_task_events_write_to_file(list, task_events_file);
            mir_task_list_destroy(list);
        }

        // Close task_events file
        fclose(task_events_file);
    }/*}}}*/

    // Kill workers
    MIR_DEBUG(MIR_DEBUG_STR "Killing workers ...\n");
    //MIR_INFORM(MIR_INFORM_STR "Killing workers ...\n");
    __sync_fetch_and_add(&g_sig_worker_alive, runtime->num_workers - 1);
    for (int i = 0; i < runtime->num_workers; i++)
    {
        struct mir_worker_t* worker = &runtime->workers[i];
        mir_lock_unset(&worker->sig_die);
        __sync_synchronize();
    }
    // Wait for workers to signal dead
    //MIR_INFORM(MIR_INFORM_STR "Waiting until workers are dead ...\n");
wait_dead:
    MIR_ASSERT(g_sig_worker_alive < runtime->num_workers);
    if (g_sig_worker_alive == 0) goto dead;
    __sync_synchronize();
    goto wait_dead;
dead:
    //MIR_INFORM(MIR_INFORM_STR "Workers are dead.\n");

    // Deinit memory allocation policy
    MIR_DEBUG(MIR_DEBUG_STR "Stopping memory distributer ...\n");
#ifdef MIR_MEM_POL_ENABLE
    mir_mem_pol_destroy();
#endif

    // Deinit scheduling policy
    MIR_DEBUG(MIR_DEBUG_STR "Stopping scheduler ...\n");
    runtime->sched_pol->destroy();

    // Deinit architecture
    MIR_DEBUG(MIR_DEBUG_STR "Releasing architecture memory ...\n");
    runtime->arch->destroy();

    // OpenMP support
    // Destroy unnamed omp critical lock
    mir_lock_destroy(&runtime->omp_critsec_lock);

    // Release runtime memory
    MIR_DEBUG(MIR_DEBUG_STR "Releasing runtime memory ...\n");
    mir_free_int(runtime->worker_cpu_map, sizeof(uint16_t) * runtime->arch->num_cores);
    // We let this linger to detect multiple non-nested calls to mir_create. // mir_free_int(runtime, sizeof(struct mir_runtime_t));

    // Report allocated memory (unfreed memory)
    MIR_DEBUG(MIR_DEBUG_STR "Total unfreed memory=%" MIR_FORMSPEC_UL " bytes\n", mir_get_allocated_memory());

shutdown:
    MIR_DEBUG(MIR_DEBUG_STR "Shutdown complete!\n");
    return;
}/*}}}*/

