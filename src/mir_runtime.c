/*
 *  Description: This is the MIR runtime system. The name MIR is inspired by MIR publishers, whose books I enjoyed during my childhood. I learned later on in life that MIR in Russian means peace. 
 *  Author:  Ananya Muddukrishna
 */

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

#include "mir_runtime.h"
#include "mir_debug.h"
#include "mir_defines.h"
#include "mir_lock.h"
#include "mir_memory.h"
#include "mir_types.h"
#include "mir_recorder.h"
#include "mir_perf.h"
#include "mir_worker.h"
#include "mir_sched_pol.h"
#include "mir_arch.h"
#include "mir_mem_pol.h"
#include "mir_utils.h"

#ifndef __tile__
#include <papi.h>
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
    int rval = pthread_key_create(&runtime->worker_index, NULL); 
    if (rval != 0)
        MIR_ABORT(MIR_ERROR_STR "Unable to create worker TLS key!\n");

    // Flags
    runtime->sig_dying = 0;
    runtime->enable_stats = 0;
    runtime->enable_recorder = 0;
    runtime->enable_dependence_resolver = 0;
}/*}}}*/

void mir_postconfig_init()
{/*{{{*/
    // Init time
    runtime->init_time = mir_get_cycles();

    // Scheduling policy
    runtime->sched_pol->create();
    MIR_DEBUG(MIR_DEBUG_STR "Sched pol set to %s\n", runtime->sched_pol->name);

    // Recorder
    if(runtime->enable_recorder == 1)
    {
#ifdef MIR_RECORDER_USE_HW_PERF_COUNTERS
#ifndef __tile__
        // PAPI
        int retval = PAPI_library_init( PAPI_VER_CURRENT );
        if ( retval != PAPI_VER_CURRENT )
            MIR_ABORT(MIR_ERROR_STR "PAPI_library_init failed!");
#endif
#endif
    }

    // Workers
    __sync_fetch_and_add(&g_sig_worker_alive, runtime->num_workers-1);
#ifdef __tile__
    __asm__ __volatile__ ("mf;"::);
#else
    __sync_synchronize();
#endif
    for (uint32_t i=0; i<runtime->num_workers; i++) 
    {
        // FIXME: Master thread stack size, how to set?
        struct mir_worker_t* worker = &runtime->workers[i];
        worker->id = i;
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

    MIR_DEBUG(MIR_DEBUG_STR "Initialization complete!\n");
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
                case 'l':
                    if(tok[2] == '=')
                    {
                        char* s = tok+3;
                        int ps_sz = atoi(s) * 1024 *1024;
                        if(0 == mir_pstack_set_size(ps_sz))
                            MIR_DEBUG(MIR_DEBUG_STR "Process stack size set to %d MB!\n", ps_sz);
                        else
                            MIR_DEBUG(MIR_DEBUG_STR "Could not set process stack size to %d MB!\n", ps_sz);
                    }
                    else
                    {
                        MIR_ABORT(MIR_ERROR_STR "Incorrect MIR_CONF parameter [%c]\n", c);
                    }
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
}/*}}}*/

void mir_destroy()
{/*{{{*/
    MIR_DEBUG(MIR_DEBUG_STR "Shutting down ...\n");

    // Check if workers are free
    mir_worker_check_done();

    // Freeze workers
    runtime->sig_dying = 1;
    __sync_synchronize();

    // Shutdown recorders
    if(runtime->enable_recorder == 1)
    {/*{{{*/
        for (int i=0; i<runtime->num_workers; i++)
        {
            mir_recorder_dump_to_file(runtime->workers[i].recorder);
            mir_recorder_destroy(runtime->workers[i].recorder);
        }
    }/*}}}*/

    // Dump statistics
    if(runtime->enable_stats == 1) 
    {/*{{{*/
        // Open stats file
        FILE* stats_file = NULL;
        stats_file = fopen(MIR_STATS_FILE_NAME, "w");
        if(!stats_file)
            MIR_ABORT(MIR_ERROR_STR "Cannot open stats file %s for writing!\n", MIR_STATS_FILE_NAME);

        // Write all worker status counters to stats file
        for(int i=0; i<runtime->num_workers; i++) 
        {
            struct mir_worker_status_t* status = runtime->workers[i].status;
            mir_worker_status_dump_to_file(status, stats_file);
        }

        // Close stats file
        fclose(stats_file);
    }/*}}}*/

    // Kill workers
    for(int i=0; i<runtime->num_workers; i++) 
    {
        struct mir_worker_t* worker = &runtime->workers[i];
        mir_lock_unset(&worker->sig_die);
    }
    __sync_synchronize();

    // Deinit memory allocation policy
    mir_mem_pol_destroy();

    // Deinit scheduling policy
    runtime->sched_pol->destroy();

    // Deinit architecture
    runtime->arch->destroy();

    // Release runtime memory
    mir_free_int(runtime, sizeof(struct mir_runtime_t));

    // Report allocated memory (unfreed memory)
    MIR_DEBUG(MIR_DEBUG_STR "Total unfreed memory=%" MIR_FORMSPEC_UL " bytes\n", mir_get_allocated_memory());

    MIR_DEBUG(MIR_DEBUG_STR "Shutdown complete!\n");
    return;
}/*}}}*/

