#ifndef MIR_WORKER_H
#define MIR_WORKER_H 1

#include <pthread.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

#include "mir_defines.h"
#include "mir_lock.h"
#include "mir_recorder.h"
#include "mir_task.h"
#include "mir_types.h"

BEGIN_C_DECLS

extern uint32_t g_worker_status_board;
extern uint32_t g_num_tasks_waiting;

struct mir_worker_statistics_t {
    uint16_t id;
    uint32_t num_tasks_created;
    // FIXME: Owned? Think of a better word. Well, owned is opposite of stolen.
    uint32_t num_tasks_owned;
    uint32_t num_tasks_stolen;
    uint32_t num_tasks_inlined;
    // These are specific to tasks with communication costs
    uint32_t num_comm_tasks;
    unsigned long total_comm_cost;
    unsigned long lowest_comm_cost;
    unsigned long highest_comm_cost;
    uint32_t* num_comm_tasks_stolen_by_diameter;
};

struct mir_worker_t {
    pthread_t pthread;
    uint16_t id;
    uint16_t cpu_id;
    uint16_t bias;
    uint32_t backoff_us;
    struct mir_lock_t sig_die;
    int sig_dying;
    struct mir_task_t* current_task;
    struct mir_worker_statistics_t* statistics;
    struct mir_recorder_t* recorder;
    // The private task queue holds worker-specific tasks
    // such as OMP for loop and parallel block tasks.
    // It is crucial that tasks are retreived in FIFO order from the private task queue.
    struct mir_task_queue_t* private_queue;
    // For task statistics
    struct mir_task_list_t* task_list;
};

void* idle_task_func(void* arg);

void mir_worker_update_bias(struct mir_worker_t* worker);

void mir_worker_master_init(struct mir_worker_t* worker);

void mir_worker_local_init(struct mir_worker_t* worker);

void mir_worker_do_work(struct mir_worker_t* worker, int backoff);

void mir_worker_check_done();

struct mir_worker_t* mir_worker_get_context();

void mir_worker_statistics_init(struct mir_worker_statistics_t* statistics);

void mir_worker_statistics_destroy(struct mir_worker_statistics_t* statistics);

void mir_worker_statistics_update_comm_cost(struct mir_worker_statistics_t* statistics, unsigned long comm_cost);

void mir_worker_statistics_write_header_to_file(FILE* file);

void mir_worker_statistics_write_to_file(const struct mir_worker_statistics_t* statistics, FILE* file);

void mir_worker_update_task_list(struct mir_worker_t* worker, struct mir_task_t* task);

void mir_worker_push(struct mir_worker_t* worker, struct mir_task_t* task);

END_C_DECLS
#endif
