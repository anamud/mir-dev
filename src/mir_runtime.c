#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <getopt.h>
#include <sys/shm.h>

#include "mir_runtime.h"
#include "mir_defines.h"
#include "mir_memory.h"
#include "mir_mem_pol.h"

// The global runtime object
struct mir_runtime_t* runtime = NULL;

// FIXME: Make this per-worker
uint32_t g_sig_worker_alive = 0;

// Extern global data.
extern uint64_t g_tasks_uidc;
extern uint32_t g_num_tasks_waiting;
extern uint32_t g_worker_status_board;
extern uint64_t g_total_allocated_memory;

static void mir_preconfig_init(int num_workers)
{ /*{{{*/
    MIR_DEBUG("Starting initialization ...");

    // Initialization control
    runtime->init_count = 1;
    runtime->destroyed = 0;

    // Arch
    runtime->arch = mir_arch_create_by_query();
    MIR_ASSERT(runtime->arch != NULL);
    MIR_ASSERT(runtime->arch->num_cores <= MIR_WORKER_MAX_COUNT);
    MIR_DEBUG("Architecture set to %s.", runtime->arch->name);

    // Scheduling policy
    runtime->sched_pol = mir_sched_pol_get_by_name(MIR_SCHED_POL_DEFAULT);
    MIR_ASSERT(runtime->sched_pol != NULL);

    // Memory allocation policy
    mir_mem_pol_create();

    // Workers
    MIR_ASSERT_STR(num_workers <= runtime->arch->num_cores, "Cannot create more workers than number of available cores.");
    runtime->num_workers = num_workers == 0 ? runtime->arch->num_cores : num_workers;
    runtime->worker_cpu_map = mir_malloc_int(sizeof(uint16_t) * runtime->arch->num_cores);
    MIR_CHECK_MEM(runtime->worker_cpu_map != NULL);
    for (int i = 0; i < runtime->num_workers; i++)
        runtime->worker_cpu_map[i] = i;
    int ret_val = pthread_key_create(&runtime->worker_index, NULL);
    MIR_ASSERT_STR(ret_val == 0, "Call to pthread_key_create failed.");

    // OpenMP support
    // This is the unnamed critical section lock
    mir_lock_create(&runtime->omp_critsec_lock);
    runtime->omp_for_schedule = OFS_STATIC;
    runtime->omp_for_chunk_size = 0;
    parse_omp_schedule();

    // Flags
    runtime->sig_dying = 0;
    runtime->enable_worker_stats = 0;
    runtime->enable_task_stats = 0;
    runtime->enable_recorder = 0;
    runtime->enable_ofp_handshake = 0;
    runtime->task_inlining_limit = MIR_INLINE_TASK_DURING_CREATION;
} /*}}}*/

static void mir_postconfig_init()
{ /*{{{*/
    // Init time
    runtime->init_time = mir_get_cycles();

    // Scheduling policy
    runtime->sched_pol->create();
    MIR_DEBUG("Task scheduling policy set to %s.", runtime->sched_pol->name);

    // Enable communication between outline function profiler and MIR
    if (runtime->enable_ofp_handshake == 1) {
        /*{{{*/
        // Create shared memory
        runtime->ofp_shmid = shmget(MIR_OFP_SHM_KEY, MIR_OFP_SHM_SIZE, IPC_CREAT | 0666);
        if (runtime->ofp_shmid < 0)
            MIR_LOG_ERR("Call to shmget failed [errorid = %d].", runtime->ofp_shmid);

        // Attach
        runtime->ofp_shm = shmat(runtime->ofp_shmid, NULL, 0);
        MIR_ASSERT(runtime->ofp_shm != NULL);
    } /*}}}*/
    else {
        /*{{{*/
        runtime->ofp_shmid = -1;
        runtime->ofp_shm = NULL;
    } /*}}}*/

    // Recorder
    if (runtime->enable_recorder == 1) {
/*{{{*/
#ifdef MIR_RECORDER_USE_HW_PERF_COUNTERS
#ifndef __tile__
        // PAPI
        int retval = PAPI_library_init(PAPI_VER_CURRENT);
        if (retval != PAPI_VER_CURRENT)
            MIR_LOG_ERR("PAPI_library_init failed [retval = %d != (PAPI_VER_CURRENT = %d)].", retval, PAPI_VER_CURRENT);
        retval = PAPI_thread_init(pthread_self);
        if (retval != PAPI_OK)
            MIR_LOG_ERR("PAPI_thread_init failed [retval = %d].", retval);
#endif
#endif
    } /*}}}*/

    // Workers
    MIR_DEBUG("Number of workers set to %d.", runtime->num_workers);
    __sync_fetch_and_add(&g_sig_worker_alive, runtime->num_workers - 1);
#ifdef __tile__
    __asm__ __volatile__("mf;" ::);
#else
    __sync_synchronize();
#endif
#ifdef MIR_WORKER_EXPLICIT_BIND
    const char* worker_cpu_map_str = getenv("MIR_WORKER_CPU_MAP");
    if (worker_cpu_map_str)
        if (strlen(worker_cpu_map_str) > 0) {
            MIR_DEBUG("Reading worker to cpu map ...");

            // Copy to buf
            char str[MIR_LONG_NAME_LEN];
            strcpy(str, worker_cpu_map_str);

            // Parse for cpu ids
            uint16_t tok_cnt = 0;
            char* tok = strtok(str, ",");
            while (tok != NULL && tok_cnt < runtime->num_workers) {
                int id;
                sscanf(tok, "%d", &id);
                //MIR_DEBUG("Read token %d ...", id);
                runtime->worker_cpu_map[tok_cnt] = (uint16_t)id;
                tok_cnt++;
                tok = strtok(NULL, ",");
            }
            MIR_ASSERT(tok_cnt == runtime->num_workers);
        }
#endif
    for (uint32_t i = 0; i < runtime->num_workers; i++) {
        // FIXME: Master thread stack size, how to set?
        struct mir_worker_t* worker = &runtime->workers[i];
        worker->id = i;
        worker->cpu_id = runtime->worker_cpu_map[i];
        if (worker->id == 0)
            mir_worker_local_init(worker);
        else
            mir_worker_master_init(worker);
    }

    // Wait for workers to signal alive
    MIR_ASSERT(g_sig_worker_alive < runtime->num_workers);
wait_alive:
    if (g_sig_worker_alive == 0)
        goto alive;
    __sync_synchronize();
    goto wait_alive;
alive:

    // Global taskwait counter
    runtime->ctwc = mir_twc_create();
    runtime->num_children_tasks = 0;

    // Memory allocation policy
    // Init memory distributer only after workers are bound.
    // Node restrictions can then be correctly inferred.
    mir_mem_pol_init();

    MIR_DEBUG("Initialization complete.");
} /*}}}*/

static inline void print_help()
{ /*{{{*/
    // Here all configuration components
    // ... should define the intention
    // ... of their config symbols

    fprintf(stderr, "Valid options in MIR_CONF environment variable ...\n"
                              "-h (--help) print this help message\n"
                              "-w <int> (--workers) number of workers\n"
                              "-s <str> (--schedule) task scheduling policy. Choose among central, central-stack, ws, ws-de and numa.\n"
                              "-m <str> (--memory-policy) memory allocation policy. Choose among coarse, fine and system.\n"
                              "--inlining-limit=<int> task inlining limit based on number of tasks per worker.\n"
                              "--stack-size=<int> worker stack size in MB\n"
                              "--queue-size=<int> task queue capacity\n"
                              "--numa-footprint=<int> for numa scheduling policy. Indicates data footprint size in bytes below which task is dealt to worker's private queue.\n"
                              "--worker-stats collect worker statistics\n"
                              "--task-stats collect task statistics\n"
                              "-r (--recorder) enable worker recorder\n"
                              "-p (--profiler) enable communication with Outline Function Profiler. Note: This option is supported only for single-worker execution!\n");
} /*}}}*/

static void mir_config()
{ /*{{{*/
    // Get MIR_CONF environment string
    const char* conf_str = getenv("MIR_CONF");
    if (!conf_str || strlen(conf_str) == 0)
        return;

    // Copy to buffer
    char* tmp_buf = strdup(getenv("MIR_CONF"));

    // Parse arguments using GNU getopt
    int conf_argc = 1;
    char* conf_argv[MIR_SBUF_SIZE];
    conf_argv[0] = "MIR_CONF";
    char* tok = strtok(tmp_buf, " ");
    while (tok) {
        conf_argc++;
        MIR_ASSERT_STR(conf_argc <= MIR_SBUF_SIZE, "MIR_CONF string is larger than %d.", MIR_SBUF_SIZE);
        // Copy to buffer
        conf_argv[conf_argc - 1] = strdup(tok);
        tok = strtok(NULL, " ");
    }
    int c;
    while (1) {
        static struct option long_options[] = {
            { "workers", required_argument, 0, 'w' },
            { "schedule", required_argument, 0, 's' },
            { "memory-policy", required_argument, 0, 'm' },
            { "stack-size", required_argument, 0, 0 },
            { "inlining-limit", required_argument, 0, 0 },
            { "numa-footprint", required_argument, 0, 0 },
            { "queue-size", required_argument, 0, 0 },
            { "help", no_argument, 0, 'h' },
            { "profiling", no_argument, 0, 'p' },
            { "recorder", no_argument, 0, 'r' },
            { "worker-stats", no_argument, 0, 0 },
            { "task-stats", no_argument, 0, 0 },
            { 0, 0, 0, 0 }
        };

        int option_index = 0;

        c = getopt_long(conf_argc, conf_argv, "w:s:m:hpr",
            long_options, &option_index);

        if (c == -1)
            break;

        switch (c) {
        case 0:
            if (0 == strcmp(long_options[option_index].name, "inlining-limit")) {
                runtime->task_inlining_limit = atoi(optarg);
                MIR_DEBUG("Task inlining limit set to %u.", runtime->task_inlining_limit);
            }
            else if (0 == strcmp(long_options[option_index].name, "stack-size")) {
                int ps_sz = atoi(optarg) * 1024 * 1024;
                MIR_ASSERT_STR(ps_sz > 0, "Stack size should be greater than 0.");
                MIR_ASSERT_STR(0 == mir_pstack_set_size(ps_sz), "Call to mir_pstack_set_size failed.");
                MIR_DEBUG("Process stack size set to %d bytes.", ps_sz);
            }
            else if (0 == strcmp(long_options[option_index].name, "worker-stats")) {
                runtime->enable_worker_stats = 1;
                MIR_DEBUG("Worker statistics collection is enabled.");
            }
            else if (0 == strcmp(long_options[option_index].name, "task-stats")) {
                runtime->enable_task_stats = 1;
                MIR_DEBUG("Task statistics collection is enabled.");
            }
            else if (0 == strcmp(long_options[option_index].name, "queue-size")) {
                runtime->sched_pol->queue_capacity = atoi(optarg);
                MIR_ASSERT_STR(runtime->sched_pol->queue_capacity > 0, "Queue capacity should be greater than 0.");
                MIR_DEBUG("Task queue capacity set to %d.", runtime->sched_pol->queue_capacity);
            }
            else if (0 == strcmp(long_options[option_index].name, "numa-footprint")) {
#ifdef MIR_MEM_POL_ENABLE
                g_numa_schedule_footprint_config = atoi(optarg);
                MIR_ASSERT_STR(g_numa_schedule_footprint_config > 0, "NUMA scheduling policy footprint argument should be greater than 0.");
                MIR_DEBUG("Footprint limit for numa scheduling policy set to %zd.", g_numa_schedule_footprint_config);
#else
                MIR_LOG_ERR("MIR built without HAVE_LIBNUMA enabled.");
#endif
            }
            else {
                MIR_LOG_ERR("Unrecognized option: %s.", long_options[option_index].name);
            }
            break;

        case '1':
        case '2':
            MIR_LOG_ERR("Incorrect MIR_CONF parameter %c.", c);
            break;

        case 'w':
            runtime->num_workers = atoi(optarg);
            if (runtime->num_workers > runtime->arch->num_cores)
                MIR_LOG_ERR("Cannot configure more workers (%d) than number of cores (%d).",
                    runtime->num_workers, runtime->arch->num_cores);
            break;

        case 's':
            runtime->sched_pol = mir_sched_pol_get_by_name(optarg);
            if (runtime->sched_pol == NULL)
                MIR_LOG_ERR("Cannot select %s scheduling policy.", optarg);
            break;

        case 'm':
            mir_mem_pol_config(optarg);
            break;

        case 'p':
            runtime->enable_ofp_handshake = 1;
            MIR_DEBUG("OFP handshake mode is enabled.");
            break;

        case 'r':
            runtime->enable_recorder = 1;
            MIR_DEBUG("Recorder is enabled.");
            break;

        case 'h':
            print_help();
            break;

        case '?':
            // Add a MIR_LOG_ERR here if unrecognized options are fatal.
            break;

        default:
            MIR_LOG_ERR("Incorrect MIR_CONF parameter %c.", c);
            break;
        }
    }

    // Free string buffers.
    free(tmp_buf);
    while (conf_argc != 1) {
        free(conf_argv[conf_argc - 1]);
        conf_argc--;
    }

    // Check if arguments are vaild.
    if (runtime->num_workers != 1 && runtime->enable_ofp_handshake == 1)
        MIR_LOG_ERR("Cannot enable OFP handshake mode when number of workers (%d) != 1.", runtime->num_workers);
} /*}}}*/

void mir_create_int(int num_workers)
{ /*{{{*/
    // Create only if first call
    if (runtime != NULL) {
        MIR_ASSERT_STR(num_workers == runtime->num_workers || num_workers == 0, "Runtime system is already created. Number of workers requested differs from first request.");
        MIR_ASSERT(runtime->destroyed == 0);
        __sync_fetch_and_add(&(runtime->init_count), 1);
        return;
    }

    // Create the global runtime
    runtime = mir_malloc_int(sizeof(struct mir_runtime_t));
    MIR_CHECK_MEM(runtime != NULL);

    // Set defaults and other stuff
    mir_preconfig_init(num_workers);

    // Set configurable parameters from command line
    mir_config();

    // Initialize
    mir_postconfig_init();

    atexit(mir_destroy);

    // Set a marking event
    MIR_RECORDER_EVENT(NULL, 0);
} /*}}}*/

void mir_create()
{ /*{{{*/
    mir_create_int(0);
} /*}}}*/

/**
    Reduces the nesting level counter but leaves the RTS mostly alive.

    Call @mir_destroy@ for proper destruction.
*/
void mir_soft_destroy()
{ /*{{{*/
    MIR_ASSERT(runtime->init_count > 0);
    __sync_fetch_and_sub(&(runtime->init_count), 1);
} /*}}}*/

void mir_destroy()
{ /*{{{*/
    // Destroy only once. Multiple calls happen if the user inserts
    // explicit calls to mir_destroy() in the program.
    if (runtime == NULL)
        return;
    //MIR_ASSERT(runtime->destroyed == 0);
    if (runtime->destroyed == 1)
        return;

    // Destory only if corresponding to first call to mir_create
    __sync_fetch_and_sub(&(runtime->init_count), 1);
    if (runtime->init_count <= 0)
        runtime->destroyed = 1;
    else
        return;

    // Set a marking event
    MIR_RECORDER_EVENT(NULL, 0);

    // Check if workers are free
    MIR_DEBUG("Checking if workers are done ...");
    mir_worker_check_done();

    // Announce destruction
    runtime->sig_dying = 1;

    // Freeze workers
    for (int i = 0; i < runtime->num_workers; i++)
        runtime->workers[i].sig_dying = 1;
    __sync_synchronize();
    MIR_DEBUG("Workers are done. Sent die signal.");

    // Shutdown recorders
    if (runtime->enable_recorder == 1) {
        /*{{{*/
        MIR_DEBUG("Shutting down recorders ...");
        for (int i = 0; i < runtime->num_workers; i++) {
            mir_recorder_write_to_file(runtime->workers[i].recorder);
            mir_recorder_destroy(runtime->workers[i].recorder);
        }
    } /*}}}*/

    // Dump worker statistics
    if (runtime->enable_worker_stats == 1) {
        /*{{{*/
        MIR_DEBUG("Dumping worker stats ...");
        // Open stats file
        FILE* stats_file;
        stats_file = fopen(MIR_WORKER_STATS_FILE_NAME, "w");
        if (!stats_file)
            MIR_LOG_ERR("Cannot open worker stats file %s for writing!", MIR_WORKER_STATS_FILE_NAME);

        // Write header
        mir_worker_statistics_write_header_to_file(stats_file);
        // Write all worker statistics counters to stats file
        for (int i = 0; i < runtime->num_workers; i++) {
            struct mir_worker_statistics_t* statistics = runtime->workers[i].statistics;
            mir_worker_statistics_write_to_file(statistics, stats_file);
            // FIXME: Maybe this should be done by the worker
            mir_worker_statistics_destroy(statistics);
        }

        // Close stats file
        fclose(stats_file);
    } /*}}}*/

    // Dump task statistics
    if (runtime->enable_task_stats == 1) {
        /*{{{*/
        MIR_DEBUG("Dumping task statistics ...");
        // Open stats file
        FILE* task_statistics_file;
        task_statistics_file = fopen(MIR_TASK_STATS_FILE_NAME, "w");
        if (!task_statistics_file)
            MIR_LOG_ERR("Cannot open task statistics file %s for writing!", MIR_TASK_STATS_FILE_NAME);

        // Write header
        mir_task_stats_write_header_to_file(task_statistics_file);
        // Write per-worker task statistics to file
        for (int i = 0; i < runtime->num_workers; i++) {
            struct mir_task_list_t* list = runtime->workers[i].task_list;
            mir_task_stats_write_to_file(list, task_statistics_file);
            mir_task_list_destroy(list);
        }

        // Close task_statistics file
        fclose(task_statistics_file);
    } /*}}}*/

    // Kill workers
    MIR_DEBUG("Killing workers ...");
    __sync_fetch_and_add(&g_sig_worker_alive, runtime->num_workers - 1);
    for (int i = 0; i < runtime->num_workers; i++) {
        struct mir_worker_t* worker = &runtime->workers[i];
        mir_lock_unset(&worker->sig_die);
        __sync_synchronize();
    }
// Wait for workers to signal dead
wait_dead:
    MIR_ASSERT(g_sig_worker_alive < runtime->num_workers);
    if (g_sig_worker_alive == 0)
        goto dead;
    __sync_synchronize();
    goto wait_dead;
dead:

    // Deinit memory allocation policy
    mir_mem_pol_destroy();

    // Deinit scheduling policy
    MIR_DEBUG("Stopping scheduler ...");
    runtime->sched_pol->destroy();

    // Deinit architecture
    MIR_DEBUG("Releasing architecture memory ...");
    runtime->arch->destroy();

    // OpenMP support
    // Destroy unnamed omp critical lock
    mir_lock_destroy(&runtime->omp_critsec_lock);

    // Release runtime memory
    MIR_DEBUG("Releasing runtime memory ...");
    mir_free_int(runtime->worker_cpu_map, sizeof(uint16_t) * runtime->arch->num_cores);
    // We let this linger to detect multiple non-nested calls to mir_create. // mir_free_int(runtime, sizeof(struct mir_runtime_t));

    // Report allocated memory (unfreed memory)
    MIR_DEBUG("Total unfreed memory=%" MIR_FORMSPEC_UL " bytes.", mir_get_allocated_memory());

shutdown:
    // Reset global data.
    runtime = NULL;
    g_sig_worker_alive = 0;
    g_num_tasks_waiting = 0;
    g_tasks_uidc = MIR_TASK_ID_START + 1;
    g_worker_status_board = 0;
    g_total_allocated_memory = 0;

    MIR_DEBUG("Shutdown complete.");
    return;
} /*}}}*/

