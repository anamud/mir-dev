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
#include "mir_types.h"

BEGIN_C_DECLS 

// Task identifer based on position in task graph
typedef unsigned int* TGPID;
void tgpid_fprint(TGPID id, FILE* fp, int cr);

// The task function pointer type
typedef void* (*mir_tfunc_t)(void*);

// Forward declaration
struct mir_task_t;

// The task wait counter
struct mir_twc_t 
{/*{{{*/
    unsigned long count;
    unsigned long num_passes;
    struct mir_task_t* parent;
    unsigned int count_per_worker[MIR_WORKER_MAX_COUNT];
};/*}}}*/

// The task
struct mir_task_t
{/*{{{*/
    mir_tfunc_t func;
#ifdef MIR_TASK_VARIABLE_DATA_SIZE
    char* data;
#else
    char data[MIR_TASK_DATA_MAX_SIZE];
#endif
    size_t data_size;
    mir_id_t id;
    TGPID tgpid;
    unsigned int num_children;
    struct mir_twc_t* twc;
    struct mir_twc_t* ctwc; // Sync counter for children
    unsigned long comm_cost;
    char name[MIR_SHORT_NAME_LEN];
    struct mir_task_t* parent;
    uint16_t core_id; 

    // Flags
    uint32_t done;
    uint32_t taken;

    // Data footprint
    struct mir_data_footprint_t* data_footprints;
    uint32_t num_data_footprints;
    struct mir_mem_node_dist_t* dist_by_access_type[MIR_DATA_ACCESS_NUM_TYPES];
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

struct mir_task_t* mir_task_create(mir_tfunc_t tfunc, void* data, size_t data_size, unsigned int num_data_footprints, struct mir_data_footprint_t* data_footprints, const char* name);

struct mir_task_t* mir_task_create_on(mir_tfunc_t tfunc, void* data, size_t data_size, unsigned int num_data_footprints, struct mir_data_footprint_t* data_footprints, const char* name, unsigned int target);

void mir_task_destroy(struct mir_task_t* task);

void mir_task_execute(struct mir_task_t* task);

#ifdef MIR_MEM_POL_ENABLE
struct mir_mem_node_dist_t* mir_task_get_footprint_dist(struct mir_task_t* task, mir_data_access_t access);
#endif

struct mir_twc_t* mir_twc_create();

void mir_twc_wait();

void mir_task_wait();

END_C_DECLS
#endif
