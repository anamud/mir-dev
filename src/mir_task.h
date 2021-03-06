#ifndef MIR_TASK_H
#define MIR_TASK_H 1

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>

#include "mir_worker.h"
#include "mir_types.h"
#include "mir_defines.h"
#include "mir_types.h"
#include "mir_utils.h"
#include "mir_loop.h"
#include "mir_team.h"
#include "mir_twc.h"

BEGIN_C_DECLS

// For task statistics collection
// FIXME: Naming, extra indirection
struct mir_task_list_t;
struct mir_task_list_t {
    struct mir_task_t* task;
    struct mir_task_list_t* next;
};

/*PUB_INT_BASE_DECL_BEGIN*/
enum mir_data_access_t {
    MIR_DATA_ACCESS_READ = 0,
    MIR_DATA_ACCESS_WRITE,
    MIR_DATA_ACCESS_NUM_TYPES
};
typedef enum mir_data_access_t mir_data_access_t;
/*PUB_INT_BASE_DECL_END*/

/*PUB_INT_DECL_BEGIN*/
struct mir_data_footprint_t {
    void* base;
    size_t type;
    uint64_t start;
    uint64_t end;
    uint64_t row_sz; // FIXME: This restricts footprints to square blocks
    mir_data_access_t data_access;
    void* part_of;
};
/*PUB_INT_DECL_END*/

static inline void data_footprint_copy(struct mir_data_footprint_t* dest, const struct mir_data_footprint_t* src)
{ /*{{{*/
    // Check
    MIR_ASSERT(src != NULL);
    MIR_ASSERT(dest != NULL);

    // Copy elements
    dest->base = src->base;
    dest->type = src->type;
    dest->start = src->start;
    dest->end = src->end;
    dest->row_sz = src->row_sz;
    dest->data_access = src->data_access;
    dest->part_of = src->part_of;
} /*}}}*/

END_C_DECLS

BEGIN_C_DECLS

// The task function pointer type
/*PUB_INT*/ typedef void* (*mir_tfunc_t)(void*);

// The task
struct mir_task_t { /*{{{*/
    mir_tfunc_t func;
#ifdef MIR_TASK_FIXED_DATA_SIZE
    char data_buf[MIR_TASK_DATA_MAX_SIZE];
#endif
    char* data;
    size_t data_size;
    mir_id_t id;
    unsigned int child_number;
    unsigned int num_children;
    unsigned long sync_pass;
    struct mir_twc_t* twc;
    struct mir_twc_t* ctwc; // Sync counter for children
    unsigned long comm_cost;
    char name[MIR_SHORT_NAME_LEN];
    char metadata[MIR_SHORT_NAME_LEN];
    struct mir_task_t* parent;
    struct mir_task_t* predecessor;
    uint16_t cpu_id;
    uint64_t create_instant;
    uint64_t exec_resume_instant;
    uint64_t exec_end_instant;
    uint64_t exec_cycles;
    uint64_t creation_cycles; // Creation cost borne by parent.
    uint64_t overhead_cycles;
    uint32_t queue_size_at_pop;
    struct mir_loop_des_t* loop;
    struct mir_omp_team_t* team;

    // Flags
    uint32_t done;
    uint32_t taken;

    // Data footprint
    struct mir_data_footprint_t* data_footprints;
    uint32_t num_data_footprints;
    struct mir_mem_node_dist_t* dist_by_access_type[MIR_DATA_ACCESS_NUM_TYPES];
}; /*}}}*/

#ifdef MIR_TASK_DEBUG
static void T_DBG(char* msg, struct mir_task_t* t)
{ /*{{{*/
    fprintf(stderr, "%lu\t#%p task %" MIR_FORMSPEC_UL " id %u done\t: %s\n", (unsigned long)pthread_self(),
        t, t->id.uid, t->done,
        msg);
} /*}}}*/
#else
#define T_DBG(x, y)
#endif

/*PUB_INT*/ void mir_task_create(mir_tfunc_t tfunc, void* data, size_t data_size, unsigned int num_data_footprints, struct mir_data_footprint_t* data_footprints, const char* name);

struct mir_task_t* mir_task_create_twin(char *name, struct mir_task_t* task, char *str);

struct mir_task_t* mir_task_create_common(mir_tfunc_t tfunc, void* data, size_t data_size, unsigned int num_data_footprints, const struct mir_data_footprint_t* data_footprints, const char* name, struct mir_omp_team_t* myteam, struct mir_loop_des_t* loopdes, struct mir_task_t* parent);

void mir_task_create_on_worker(mir_tfunc_t tfunc, void* data, size_t data_size, unsigned int num_data_footprints, struct mir_data_footprint_t* data_footprints, const char* name, struct mir_omp_team_t* myteam, struct mir_loop_des_t* loopdes, int workerid);

// TODO: Differentiate with mir_task_create_on_worker().
void mir_task_schedule_on_worker(struct mir_task_t* task, int workerid);

void mir_task_execute_prolog(struct mir_task_t* task);

void mir_task_execute_epilog(struct mir_task_t* task);

void mir_task_execute(struct mir_task_t* task);

void mir_task_write_metadata(struct mir_task_t* task, const char* metadata);

#ifdef MIR_MEM_POL_ENABLE
struct mir_mem_node_dist_t* mir_task_get_mem_node_dist(struct mir_task_t* task, mir_data_access_t access);
#endif

void mir_task_wait_int(struct mir_twc_t* twc, int newval);

/*PUB_INT*/ void mir_task_wait();

void mir_task_stats_write_header_to_file(FILE* file);

void mir_task_stats_write_to_file(struct mir_task_list_t* list, FILE* file);

void mir_task_list_destroy(struct mir_task_list_t* list);

END_C_DECLS
#endif
