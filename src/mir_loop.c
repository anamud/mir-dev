#include <stdbool.h>

#include "mir_memory.h"
#include "mir_loop.h"
#include "mir_utils.h"
#include "mir_lock.h"
#include "mir_defines.h"
#include "mir_runtime.h"
#include "mir_worker.h"

struct mir_loop_des_t* mir_new_omp_loop_desc()
{ /*{{{*/
    struct mir_loop_des_t* loop = mir_malloc_int(sizeof(struct mir_loop_des_t));
    MIR_CHECK_MEM(loop != NULL);
    mir_lock_create(&(loop->lock));
    loop->init = 0;

    return loop;
} /*}}}*/

void mir_omp_loop_desc_init(struct mir_loop_des_t* loop, long start, long end,
                            long incr, long chunk_size, bool use_precomp_schedule)
{ /*{{{*/
    loop->incr = incr;
    loop->next = start;
    loop->end = ((incr > 0 && start > end) || (incr < 0 && start < end)) ? start : end;
    loop->chunk_size = chunk_size;
    loop->static_trip = 0;
    loop->non_parallel_start = 0;

    if (use_precomp_schedule) {
        struct mir_worker_t* worker = mir_worker_get_context();
        MIR_ASSERT(worker != NULL);
        MIR_ASSERT(worker->current_task != NULL);

        if (worker->current_task->parent) {
            unsigned long idle_join = (strcmp(worker->current_task->parent->name, "idle_task") == 0) ? worker->current_task->twc->num_passes : worker->current_task->parent->twc->num_passes;

            char schedule_file_name[MIR_LONG_NAME_LEN + MIR_SHORT_NAME_LEN];
            char task_of[MIR_SHORT_NAME_LEN];
            sprintf(task_of, "%p", worker->current_task->func);
            sprintf(schedule_file_name, "%s/loop_%lu_%lu.schedule_opt",
                    runtime->precomp_schedule_dir,
                    strtol(task_of, NULL, 16),
                    idle_join);

            FILE* fp = fopen(schedule_file_name, "r");
            if (fp == NULL) {
                goto no_percomputed_schedule;
            }
            else {
                int num_expect = 4;
                int retval;
                if (fp) {
                    if (worker->id == 0) {
                        MIR_LOG_INFO("Using precomputed schedule file: %s.",
                                schedule_file_name);
                    }
                    unsigned long chunk_start, chunk_end, cpu_id, work_cycles;
                    while (!feof(fp)) {
                        while (num_expect == (retval = fscanf(fp,
                                                  "%lu,%lu,%lu,%lu\n",
                                                  &chunk_start,
                                                  &chunk_end,
                                                  &cpu_id,
                                                  &work_cycles))) {
                            loop->precomp_schedule_exists = true;
                            if (cpu_id == MIR_IMPOSSIBLE_CPU_ID) {
                                MIR_LOG_ERR("Precomputed schedule in file %s uses unsupported MIR_IMPOSSIBLE_CPU_ID.",
                                            schedule_file_name);
                            }
                            if (cpu_id > runtime->num_workers && cpu_id != MIR_IMPOSSIBLE_CPU_ID) {
                                MIR_LOG_ERR("Precomputed schedule in file %s has more workers than available.",
                                            schedule_file_name);
                            }
                            if (cpu_id == worker->cpu_id) {
                                struct mir_loop_schedule_t* schedule = mir_malloc_int(sizeof(struct mir_loop_schedule_t));
                                MIR_CHECK_MEM(schedule != NULL);
                                schedule->chunk_start = chunk_start;
                                schedule->chunk_end = chunk_end;
                                schedule->cpu_id = cpu_id;
                                schedule->work_cycles = work_cycles;

                                schedule->next = loop->precomp_schedule;
                                loop->precomp_schedule = schedule;
                            }
                        }
                        if (ferror(fp)) {
                            MIR_LOG_ERR("Error occured while reading precomputed schedule file: %s.",
                                        schedule_file_name);
                        }
                        else if (retval != EOF) {
                            // Skip over.
                            fscanf(fp, "%*[^\n]");
                        }
                    }
                }
                fclose(fp);
            }
        }
        else {
            goto no_percomputed_schedule;
        }
    }
    else {
        goto no_percomputed_schedule;
    }

    goto loop_init_done;

no_percomputed_schedule:
    loop->precomp_schedule = NULL;
    loop->precomp_schedule_exists = false;

loop_init_done:
    loop->init = 1;
} /*}}}*/

struct mir_loop_des_t* mir_new_omp_loop_desc_init(long start, long end, long incr, long chunk_size, bool use_precomp_schedule)
{/*{{{*/
    struct mir_loop_des_t* loop = mir_new_omp_loop_desc();
    mir_omp_loop_desc_init(loop, start, end, incr, chunk_size, use_precomp_schedule);

    return loop;
}/*}}}*/
