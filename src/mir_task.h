#ifndef MIR_TASK_H
#define MIR_TASK_H 1

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>

#include "mir_worker.h"
#include "mir_types.h"
#include "mir_defines.h"
#include "mir_data_footprint.h"

// The task function pointer type
typedef void* (*mir_tfunc_t)(void*);

// The task wait counter
struct mir_twc_t 
{/*{{{*/
    unsigned long count;
    unsigned int count_per_worker[MIR_WORKER_MAX_COUNT];
};/*}}}*/

// The task
struct mir_task_t
{/*{{{*/
    mir_tfunc_t func;
    mir_ba_sbuf_t data;
    size_t data_size;
    mir_id_t id;
    struct mir_twc_t* twc;
    unsigned long comm_cost;
    char name[MIR_SHORT_NAME_LEN];

    // Flags
    uint32_t done;

    // Data footprint
    struct mir_data_footprint_t* data_footprints;
    uint32_t num_data_footprints;
};/*}}}*/

#ifdef MIR_TASK_DEBUG
static void T_DBG(char*msg, struct mir_task_t *t) 
{/*{{{*/
    fprintf(stderr, "%lu\t#%p task %" MIR_FORMSPEC_UL " id %u done\t: %s\n",                    (unsigned long) pthread_self(),
                    t, t->id.uid, t->done,
                    msg
                    );
}/*}}}*/
#else
#define T_DBG(x,y)
#endif

struct mir_task_t* mir_task_create(mir_tfunc_t tfunc, void* data, size_t data_size, struct mir_twc_t* twc, unsigned int num_data_footprints, struct mir_data_footprint_t* data_footprints, const char* name);

void mir_task_destroy(struct mir_task_t* task);

void mir_task_schedule(struct mir_task_t* task);

void mir_task_schedule_int(struct mir_task_t* task);

void mir_task_execute(struct mir_task_t* task);

void mir_task_wait(struct mir_task_t* task);

void mir_task_wait_twc(struct mir_twc_t* twc);

struct mir_twc_t* mir_twc_create();

#endif
