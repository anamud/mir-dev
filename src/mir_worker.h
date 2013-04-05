#ifndef MIR_WORKER_H
#define MIR_WORKER_H 1

#include <pthread.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

#include "mir_defines.h"
#include "mir_lock.h"
#include "mir_recorder.h"

extern uint32_t g_worker_status_board;
extern uint32_t g_num_tasks_waiting;

//struct mir_recorder_t;
//struct mir_lock_t;

struct mir_worker_status_t
{
    uint32_t id;
    uint32_t num_tasks_spawned;
    uint32_t num_tasks_executed;
    uint32_t num_tasks_stolen;
    uint32_t num_tasks_inlined;
    unsigned long comm_cost;
    unsigned long lowest_comm_cost;
    unsigned long highest_comm_cost;
};

struct mir_worker_t
{
    pthread_t pthread;
    uint16_t id;
    struct mir_lock_t sig_die;
    struct mir_worker_status_t* status;
    struct mir_recorder_t* recorder;
};

void mir_worker_master_init(struct mir_worker_t* worker);

void mir_worker_local_init(struct mir_worker_t* worker);

void* mir_worker_loop(void* arg);

void mir_worker_do_work(struct mir_worker_t* worker);

void mir_worker_check_done();

struct mir_worker_t* mir_worker_get_context();

void mir_worker_status_reset(struct mir_worker_status_t* status);

void mir_worker_status_update_comm_cost(struct mir_worker_status_t* status, unsigned long comm_cost);

void mir_worker_status_write_header_to_file(FILE* file);

void mir_worker_status_write_to_file(struct mir_worker_status_t* status, FILE* file);
#endif 
