/*
 *  Description: This is the MIR runtime system. The name MIR is inspired by MIR publishers, whose books I enjoyed during my childhood. I learned later on in life that MIR in Russian means peace. 
 *  Author:  Ananya Muddukrishna
 */

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
#include "mir_sched_pol.h"
#include "mir_arch.h"
#include "mir_mem_pol.h"
#include "mir_utils.h"

#ifndef __tile__
#include "papi.h"
#endif

// The global runtime object
struct mir_runtime_t* runtime = NULL;

uint32_t g_sig_worker_alive = 0;

void mir_preconfig_init()
{/*{{{*/
    MIR_DEBUG(MIR_DEBUG_STR "Starting initialization ...\n");
    // Arch
    runtime->arch = mir_arch_create_by_query();
    if(runtime->arch == NULL)
        MIR_ABORT(MIR_ERROR_STR "Cannot create architecture!\n");
    else 
        MIR_DEBUG(MIR_DEBUG_STR "Architecture set to %s\n", runtime->arch->name);

    // Scheduling policy
    runtime->sched_pol = mir_sched_pol_get_by_name(MIR_SCHED_POL_DEFAULT);
    if(runtime->sched_pol == NULL)
        MIR_ABORT(MIR_ERROR_STR "Cannot select %s scheduling policy!\n", MIR_SCHED_POL_DEFAULT);

    // Memory allocation policy
    mir_mem_pol_create();

    // Workers
    runtime->num_workers = runtime->arch->num_cores;
    runtime->worker_core_map = mir_malloc_int(sizeof(uint16_t) * runtime->arch->num_cores);
    for(int i=0; i<runtime->num_workers; i++)
        runtime->worker_core_map[i] = (uint16_t) i;
    int rval = pthread_key_create(&runtime->worker_index, NULL); 
    if (rval != 0)
        MIR_ABORT(MIR_ERROR_STR "Unable to create worker TLS key!\n");

    // Flags
    runtime->sig_dying = 0;
    runtime->enable_stats = 0;
    runtime->enable_task_graph_gen = 0;
    runtime->enable_recorder = 0;
    runtime->enable_dependence_resolver = 0;
    runtime->enable_shmem_handshake = 0;
    runtime->task_inlining_limit = MIR_INLINE_TASK_DURING_CREATION;
}/*}}}*/

void mir_postconfig_init()
{/*{{{*/
    // Init time
    runtime->init_time = mir_get_cycles();

    // Scheduling policy
    runtime->sched_pol->create();
    MIR_DEBUG(MIR_DEBUG_STR "Task scheduling policy set to %s\n", runtime->sched_pol->name);

    // Shared memory for passing data and handshake signals
    // The idea is to let the PIN profiling tool and MIR communicate
    if(runtime->enable_shmem_handshake == 1)
    {/*{{{*/
        // Create shared memory
        runtime->shmid = shmget((int)(MIR_SHM_KEY), MIR_SHM_SIZE, IPC_CREAT | 0666);
        if(runtime->shmid < 0)
            MIR_ABORT(MIR_ERROR_STR "shmget failed [%d]!\n", runtime->shmid);

        // Attach
        runtime->shm = shmat(runtime->shmid, NULL, 0); 
        if (runtime->shm == NULL)
            MIR_ABORT(MIR_ERROR_STR "shmat returned NULL!\n");

        /*// Test. Write something.*/
        /*char testchar = 'A';*/
        /*for(int i=0; i<=MIR_SHM_SIZE; i++)*/
        /*{*/
            /*runtime->shm[i] = testchar;*/
            /*testchar++;*/
        /*}*/
        /*runtime->shm[MIR_SHM_SIZE-1] = '\0';*/
    }/*}}}*/
    else
    {/*{{{*/
        runtime->shmid = -1;
        runtime->shm = NULL;
    }/*}}}*/

    // Recorder
    if(runtime->enable_recorder == 1)
    {/*{{{*/
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
    __sync_fetch_and_add(&g_sig_worker_alive, runtime->num_workers-1);
#ifdef __tile__
    __asm__ __volatile__ ("mf;"::);
#else
    __sync_synchronize();
#endif
#ifdef MIR_WORKER_EXPLICIT_BIND
    const char* worker_core_map_str = getenv("MIR_WORKER_CORE_MAP");
    if(worker_core_map_str)
        if(strlen(worker_core_map_str) > 0)
        {
            MIR_DEBUG(MIR_DEBUG_STR "Reading worker to core map ...\n");

            // Copy to buf
            char str[MIR_LONG_NAME_LEN];
            strcpy(str, worker_core_map_str);

            // Parse for core ids
            uint16_t tok_cnt = 0;
            char* tok = strtok (str, ",");
            while (tok != NULL && tok_cnt < runtime->num_workers)
            {
                int id;
                sscanf (tok, "%d", &id);
                //MIR_DEBUG(MIR_DEBUG_STR "Read token %d ...\n", id);
                runtime->worker_core_map[tok_cnt] = (uint16_t) id;
                tok_cnt++;
                tok = strtok (NULL, ",");
            }
            if(tok_cnt != runtime->num_workers)
                MIR_ABORT(MIR_ERROR_STR "MIR_WORKER_CORE_MAP incompletely specified!\n");
        }
#endif
    for (uint32_t i=0; i<runtime->num_workers; i++) 
    {
        // FIXME: Master thread stack size, how to set?
        struct mir_worker_t* worker = &runtime->workers[i];
        worker->id = i;
        worker->core_id = runtime->worker_core_map[i];
        if(worker->id != 0)
            mir_worker_master_init(worker);
        if(worker->id == 0)
            mir_worker_local_init(worker);
    }

    // Wait for workers to signal alive
wait_alive:
    if (g_sig_worker_alive == 0) goto alive;
    __sync_synchronize();
    goto wait_alive;
alive:

    // Global taskwait counter
    runtime->ctwc = mir_twc_create();

    // Memory allocation policy
    // Init memory distributer only after workers are bound.
    // Node restrictions can then be correctly inferred.
    mir_mem_pol_init();

    MIR_DEBUG(MIR_DEBUG_STR "Initialization complete!\n");
}/*}}}*/

static inline void print_help()
{/*{{{*/
    // Here all configuration components
    // ... should define the intention 
    // ... of their config symbols

    MIR_INFORM(MIR_INFORM_STR "Valid options in MIR_CONF environment variable ...\n"
    "-h print this help message\n"
    "-w=<int> number of workers\n"
    "-s=<str> task scheduling policy\n"
    "-r enable recorder\n"
    "-d enable dependence resolution\n"
    "-x=<int> task inlining limit based on num tasks per worker\n"
    "-i write statistics to file\n"
    "-l=<int> stack size in MB\n"
    "-q=<int> queue capacity\n"
    "-m=<str> memory allocation policy\n"
    "-y=<csv> schedule policy specific parameters\n"
    "-g enable fork-join graph generation \n"
    "-p enable handshake with Pin profiler [Note: Supported only for a single worker!]\n"
    );
}/*}}}*/

void mir_config()
{/*{{{*/
    // Get MIR_CONF environment string 
    const char* conf_str = getenv("MIR_CONF");
    if(!conf_str)
        return;

    if(strlen(conf_str) == 0)
        return;

    // Copy to buffer
    char str[MIR_LONG_NAME_LEN];
    strcpy(str, getenv("MIR_CONF"));

    // Parse tokens
    char* tok = strtok(str, " ");
    while(tok)
    {
        if(tok[0] == '-')
        {
            char c = tok[1];
            switch(c)
            {/*{{{*/
                case 'h':
                    print_help();
                    break;
                case 'w':
                    if(tok[2] == '=')
                    {
                        char* s = tok+3;
                        runtime->num_workers = atoi(s);
                        if(runtime->num_workers > runtime->arch->num_cores)
                            MIR_ABORT(MIR_ERROR_STR "Cannot configure more workers (%d) than number of cores (%d) !\n", runtime->num_workers, runtime->arch->num_cores);
                    }
                    else
                    {
                        MIR_ABORT(MIR_ERROR_STR "Incorrect MIR_CONF parameter [%c]\n", c);
                    }
                    break;
                case 's':
                    if(tok[2] == '=')
                    {
                        char* s = tok+3;
                        runtime->sched_pol = mir_sched_pol_get_by_name(s);
                        if(runtime->sched_pol == NULL)
                            MIR_ABORT(MIR_ERROR_STR "Cannot select %s scheduling policy!\n", s);
                    }
                    else
                    {
                        MIR_ABORT(MIR_ERROR_STR "Incorrect MIR_CONF parameter [%c]\n", c);
                    }
                    break;
                case 'g':
                    /*if(runtime->num_workers == 1)*/
                    /*{*/
                        runtime->enable_task_graph_gen = 1;
                        MIR_DEBUG(MIR_DEBUG_STR "Fork-join graph generation is enabled!\n");
                    /*}*/
                    /*else*/
                    /*{*/
                        /*MIR_ABORT(MIR_ERROR_STR "Number of workers = %d != 1. Cannot enable task graph generation!\n", runtime->num_workers);*/
                    /*}*/
                    break;
                case 'r':
                    runtime->enable_recorder = 1;
                    MIR_DEBUG(MIR_DEBUG_STR "Recorder is enabled!\n");
                    break;
                case 'i':
                    runtime->enable_stats = 1;
                    break;
                case 'd':
                    MIR_ABORT(MIR_ERROR_STR "Dependence reolver not supported yet!\n");
                    // runtime->enable_dependence_resolver = 1;
                    break;
                case 'x':
                    if(tok[2] == '=')
                    {
                        char* s = tok+3;
                        runtime->task_inlining_limit = atoi(s);
                    }
                    else
                    {
                        MIR_ABORT(MIR_ERROR_STR "Incorrect MIR_CONF parameter [%c]\n", c);
                    }
                    break;
                case 'l':
                    if(tok[2] == '=')
                    {
                        char* s = tok+3;
                        int ps_sz = atoi(s) * 1024 *1024;
                        if(0 == mir_pstack_set_size(ps_sz))
                            MIR_DEBUG(MIR_DEBUG_STR "Process stack size set to %d bytes\n", ps_sz);
                        else
                            MIR_DEBUG(MIR_DEBUG_STR "Could not set process stack size to %d bytes!\n", ps_sz);
                    }
                    else
                    {
                        MIR_ABORT(MIR_ERROR_STR "Incorrect MIR_CONF parameter [%c]\n", c);
                    }
                    break;
                case 'p':
                    if(runtime->num_workers == 1)
                    {
                        runtime->enable_shmem_handshake = 1;
                        MIR_DEBUG(MIR_DEBUG_STR "Shared memory handshake mode is enabled!\n");
                    }
                    else
                    {
                        MIR_ABORT(MIR_ERROR_STR "Number of workers = %d != 1. Cannot enable shared memory handshake mode!\n", runtime->num_workers);
                    }
                    break;
                default:
                    break;
            }/*}}}*/
        }
        tok = strtok(NULL, " ");
    }

    // Pass token string to other configurable components
    mir_mem_pol_config(conf_str);
    runtime->sched_pol->config(conf_str);
    runtime->arch->config(conf_str);
}/*}}}*/

void mir_create()
{/*{{{*/
    // Create the global runtime
    runtime = mir_malloc_int(sizeof(struct mir_runtime_t));
    if( NULL == runtime )
        MIR_ABORT(MIR_ERROR_STR "Unable to create the runtime!\n");

    // Set defaults and other stuff
    mir_preconfig_init();

    // Set configurable parameters from command line
    mir_config();

    // Initialize 
    mir_postconfig_init();

    // Set a marking event
    MIR_RECORDER_EVENT(NULL,0);
}/*}}}*/

void mir_destroy()
{/*{{{*/
    // Set a marking event
    MIR_RECORDER_EVENT(NULL,0);

    MIR_DEBUG(MIR_DEBUG_STR "Shutting down ...\n");

    // FIXME: ws-de causes never-ending shutdown
    if(0 == strcmp(runtime->sched_pol->name, "ws-de"))
        goto shutdown;

    // Check if workers are free
    MIR_DEBUG(MIR_DEBUG_STR "Checking if workers are done ...\n");
    mir_worker_check_done();

    // Freeze workers
    runtime->sig_dying = 1;
    __sync_synchronize();
    MIR_DEBUG(MIR_DEBUG_STR "Workers are done. Sent die signal.\n");

    // Shutdown recorders
    if(runtime->enable_recorder == 1)
    {/*{{{*/
        MIR_DEBUG(MIR_DEBUG_STR "Shutting down recorders ...\n");
        for (int i=0; i<runtime->num_workers; i++)
        {
            mir_recorder_write_to_file(runtime->workers[i].recorder);
            mir_recorder_destroy(runtime->workers[i].recorder);
        }
    }/*}}}*/

    // Dump statistics
    if(runtime->enable_stats == 1) 
    {/*{{{*/
        MIR_DEBUG(MIR_DEBUG_STR "Dumping stats ...\n");
        // Open stats file
        FILE* stats_file = NULL;
        stats_file = fopen(MIR_STATS_FILE_NAME, "w");
        if(!stats_file)
            MIR_ABORT(MIR_ERROR_STR "Cannot open stats file %s for writing!\n", MIR_STATS_FILE_NAME);

        // Write header 
        mir_worker_status_write_header_to_file(stats_file);
        // Write all worker status counters to stats file
        for(int i=0; i<runtime->num_workers; i++) 
        {
            struct mir_worker_status_t* status = runtime->workers[i].status;
            mir_worker_status_write_to_file(status, stats_file);
            // FIXME: Maybe this should be done by the worker
            mir_worker_status_destroy(status);
        }

        // Close stats file
        fclose(stats_file);
    }/*}}}*/

    // Task graph
    if(runtime->enable_task_graph_gen == 1)
    {/*{{{*/
        MIR_DEBUG(MIR_DEBUG_STR "Writing task graph to file ...\n");
        // Open stats file
        FILE* task_graph_file = NULL;
        task_graph_file = fopen(MIR_TASK_GRAPH_FILE_NAME, "w");
        if(!task_graph_file)
            MIR_ABORT(MIR_ERROR_STR "Cannot open task_graph file %s for writing!\n", MIR_TASK_GRAPH_FILE_NAME);

        // Write header 
        mir_task_graph_write_header_to_file(task_graph_file);
        // Write all worker task graph nodes to file
        for(int i=0; i<runtime->num_workers; i++) 
        {
            struct mir_task_graph_node_t* node = runtime->workers[i].task_graph_node;
            mir_task_graph_write_to_file(node, task_graph_file);
            mir_task_graph_destroy(node);
        }

        // Close task_graph file
        fclose(task_graph_file);
    }/*}}}*/

    // Kill workers
    MIR_DEBUG(MIR_DEBUG_STR "Killing workers ...\n");
    for(int i=0; i<runtime->num_workers; i++) 
    {
        struct mir_worker_t* worker = &runtime->workers[i];
        mir_lock_unset(&worker->sig_die);
    }
    __sync_synchronize();

    // Deinit memory allocation policy
    MIR_DEBUG(MIR_DEBUG_STR "Stopping memory distributer ...\n");
    mir_mem_pol_destroy();

    // Deinit scheduling policy
    MIR_DEBUG(MIR_DEBUG_STR "Stopping scheduler ...\n");
    runtime->sched_pol->destroy();

    // Deinit architecture
    MIR_DEBUG(MIR_DEBUG_STR "Releasing architecture memory ...\n");
    runtime->arch->destroy();

    // Release runtime memory
    MIR_DEBUG(MIR_DEBUG_STR "Releasing runtime memory ...\n");
    mir_free_int(runtime->worker_core_map, sizeof(uint16_t) * runtime->arch->num_cores);
    mir_free_int(runtime, sizeof(struct mir_runtime_t));

    // Report allocated memory (unfreed memory)
    MIR_DEBUG(MIR_DEBUG_STR "Total unfreed memory=%" MIR_FORMSPEC_UL " bytes\n", mir_get_allocated_memory());

shutdown:
    MIR_DEBUG(MIR_DEBUG_STR "Shutdown complete!\n");
    return;
}/*}}}*/

