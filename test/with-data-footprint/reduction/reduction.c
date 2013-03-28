#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/time.h>
#include <math.h>

#include "mir_public_int.h"
#include "helper.h"

#define CHECK_RESULT 1

#define PAGE_SZ (4096/sizeof(uint64_t))
#define MAX_DEPTH_DEFAULT 12
#define OPR_SCALE (42)
#define SLEEP_MS 1

uint64_t* buffer = NULL;
int max_depth = MAX_DEPTH_DEFAULT;

long get_usecs(void)
{/*{{{*/
    struct timeval t;
    gettimeofday(&t, ((void *) 0));
    return t.tv_sec * 1000000 + t.tv_usec;
}/*}}}*/

void reduce_init()
{/*{{{*/
    PDBG("Allocating memory ... \n");
    uint64_t num_pages = pow(2, max_depth);
    size_t buf_size = sizeof(uint64_t) * PAGE_SZ * num_pages;
    PMSG("Memory required = %lu MB\n", buf_size/(1024*1024));
    buffer = mir_mem_pol_allocate(buf_size);
    if(!buffer)
        PABRT("Cannot allocate memory!\n");

    PDBG("Initializing memory ... \n");
    for(uint64_t i=0; i<num_pages; i++)
        for(uint64_t j=0; j<PAGE_SZ; j++)
            buffer[i*PAGE_SZ + j] = i+1;
}/*}}}*/

void reduce(uint64_t* in1, uint64_t* in2, uint64_t* out)
{/*{{{*/
    for(uint64_t i=0; i<PAGE_SZ; i++)
        out[i] = in1[i] + OPR_SCALE*in2[i];
    mir_sleep_ms(SLEEP_MS);
}/*}}}*/

struct reduce_wrapper_arg_t 
{/*{{{*/
    uint64_t* in1;
    uint64_t* in2;
    uint64_t* out;
};/*}}}*/

void reduce_wrapper(void* arg)
{/*{{{*/
    struct reduce_wrapper_arg_t* warg = (struct reduce_wrapper_arg_t*)(arg);
    reduce(warg->in1, warg->in2, warg->out);
}/*}}}*/

void for_task(uint64_t start, uint64_t end, uint64_t depth, struct mir_twc_t* twc)
{/*{{{*/
    uint64_t page = pow(2, depth + 1) * start;

    for(uint64_t j=start; j<=end; j++)
    {/*{{{*/
        // Page control
        uint64_t in1 = page;
        uint64_t in2 = page + pow(2, depth);
        uint64_t out = page;
        page += pow(2, depth+1);
        PDBG("Task %lu reads pages %lu and %lu and writes page %lu\n", j, in1, in2, out);
        
        // Create task
        {
            // Data environment
            struct reduce_wrapper_arg_t arg;
            arg.in1 = buffer + PAGE_SZ*in1;
            arg.in2 = buffer + PAGE_SZ*in2;
            arg.out = buffer + PAGE_SZ*out;

            // Data footprint
            struct mir_data_footprint_t footprints[3];
            footprints[0].base = (void*) buffer + PAGE_SZ*in1;
            footprints[0].start = 0;
            footprints[0].end = PAGE_SZ-1;
            footprints[0].type = sizeof(uint64_t);
            footprints[0].data_access = MIR_DATA_ACCESS_READ;
            footprints[0].part_of = buffer;

            footprints[1].base = (void*) buffer + PAGE_SZ*in2;
            footprints[1].start = 0;
            footprints[1].end = PAGE_SZ-1;
            footprints[1].type = sizeof(uint64_t);
            footprints[1].data_access = MIR_DATA_ACCESS_READ;
            footprints[1].part_of = buffer;

            footprints[2].base = (void*) buffer + PAGE_SZ*out;
            footprints[2].start = 0;
            footprints[2].end = PAGE_SZ-1;
            footprints[2].type = sizeof(uint64_t);
            footprints[2].data_access = MIR_DATA_ACCESS_WRITE;
            footprints[2].part_of = buffer;

            struct mir_task_t* task = mir_task_create((mir_tfunc_t) reduce_wrapper, &arg, sizeof(struct reduce_wrapper_arg_t), twc, 3, footprints, NULL);
        }
    }/*}}}*/
}/*}}}*/

struct for_task_wrapper_arg_t
{/*{{{*/
    uint64_t start;
    uint64_t end;
    uint64_t depth;
    struct mir_twc_t* twc;
};/*}}}*/

void for_task_wrapper(void* arg)
{/*{{{*/
    struct for_task_wrapper_arg_t* warg = (struct for_task_wrapper_arg_t*) arg;
    for_task(warg->start, warg->end, warg->depth, warg->twc);
}/*}}}*/

void reduce_par()
{/*{{{*/
    PDBG("Reducing in parallel ... \n");

    for(uint64_t i=0; i<max_depth; i++)
    {
        PDBG("At depth %lu ... \n", i);

        uint64_t num_tasks_at_depth = pow(2, max_depth-(i+1));
        //PDBG("Creating %lu tasks ... \n", num_tasks_at_depth);
        struct mir_twc_t* twc = mir_twc_create();

        // Split the task creation load among workers
        // Same action as worksharing omp for
        uint32_t num_workers = mir_get_num_threads();
        uint64_t num_iter = num_tasks_at_depth / num_workers;
        uint64_t num_tail_iter = num_tasks_at_depth % num_workers;
        if(num_iter > 0)
        {
            // Create prologue tasks
            for(uint32_t k=0; k<num_workers; k++)
            {
                uint64_t start = k * num_iter;
                uint64_t end = start + num_iter - 1;
                {
                    struct for_task_wrapper_arg_t arg;
                    arg.start = start;
                    arg.end = end;
                    arg.depth = i;
                    arg.twc = twc;

                    struct mir_task_t* task = mir_task_create((mir_tfunc_t) for_task_wrapper, &arg, sizeof(struct for_task_wrapper_arg_t), twc, 0, NULL, NULL);
                }
                //for_task(start, end, i);
            }
        }
        if(num_tail_iter > 0)
        {
            // Create epilogue task
            uint64_t start = num_workers * num_iter;
            uint64_t end = start + num_tail_iter - 1;
            {
                struct for_task_wrapper_arg_t arg;
                arg.start = start;
                arg.end = end;
                arg.depth = i;
                arg.twc = twc;

                struct mir_task_t* task = mir_task_create((mir_tfunc_t) for_task_wrapper, &arg, sizeof(struct for_task_wrapper_arg_t), twc, 0, NULL, NULL);
            }
            //for_task(start, end, i);
        }

        PDBG("Waiting for tasks to finish... \n");
        mir_twc_wait(twc);
    }
}/*}}}*/

int reduce_check()
{/*{{{*/
    if(OPR_SCALE != 1)
        return TEST_NOT_APPLICABLE;

    uint64_t num_pages = pow(2, max_depth);
    uint64_t check_val = (num_pages) * (num_pages+1) / 2;
    if(buffer[0] == check_val)
    {
        //PMSG("%lu = %lu\n", buffer[0], check_val);
        return TEST_SUCCESSFUL;
    }
    else
    {
        PMSG("%lu != %lu\n", buffer[0], check_val);
        return TEST_UNSUCCESSFUL;
    }
}/*}}}*/

void reduce_deinit()
{/*{{{*/
    uint64_t num_pages = pow(2, max_depth);
    size_t buf_size = sizeof(uint64_t) * PAGE_SZ * num_pages;
    mir_mem_pol_release(buffer, buf_size);
}/*}}}*/

int main(int argc, char *argv[])
{/*{{{*/
    if (argc > 2)
    {
        printf("Usage: %s depth\n", argv[0]);
        exit(0);
    }

    // Init the runtime
    mir_create();

    if(argc == 2)
        max_depth = atoi(argv[1]);

    reduce_init();

    long par_time_start = get_usecs();
    reduce_par();
    long par_time_end = get_usecs();
    double par_time = (double)( par_time_end - par_time_start) / 1000000;

    int check = TEST_NOT_PERFORMED;
    if (CHECK_RESULT)
    {
        check = reduce_check();
    }

    reduce_deinit();

    printf("%s(%d),check=%d in [SUCCESSFUL, UNSUCCESSFUL, NOT_APPLICABLE, NOT_PERFORMED],time=%f secs\n", argv[0], max_depth, check, par_time);

    // Pull down the runtime
    mir_destroy();

    return 0;
}/*}}}*/
