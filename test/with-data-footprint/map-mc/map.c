#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/time.h>
#include <math.h>
#include <unistd.h>
// NUMA
#include <numa.h>
#include <numaif.h>
#include <sys/mman.h>

#include "mir_public_int.h"
#include "helper.h"

#define OPR_SCALE (42)
//#define SLEEP_MS 0
#define LOOP_CNT 10
#define REUSE_CNT 1
#define SHOW_NUMA_STATS 1
//#define ENABLE_FAULT_IN 
//#define ENABLE_PREFETCH 1

#include <xmmintrin.h>
#define PREFETCH_T0(addr,nrOfBytesAhead) _mm_prefetch(((char *)(addr))+nrOfBytesAhead,_MM_HINT_T0) 

uint64_t** buffer = NULL;
int num_tasks = (2<<12);
int buf_sz = (2<<15);

size_t g_sum = 0;

long get_usecs(void)
{/*{{{*/
    struct timeval t;
    gettimeofday(&t, ((void *) 0));
    return t.tv_sec * 1000000 + t.tv_usec;
}/*}}}*/

/*static inline uint16_t node_of(void* addr)*/
/*{[>{{{<]*/
    /*int nodeid = -1;*/
    /*get_mempolicy(&nodeid, NULL, 0, (void*)addr, MPOL_F_NODE|MPOL_F_ADDR);*/
    /*return (uint16_t) nodeid;*/
/*}[>}}}<]*/

/*static void* interleaved_allocate(size_t sz)*/
/*{[>{{{<]*/
    /*void* addr = NULL;*/
    /*size_t new_sz = sz;*/

    /*int cur_pol;*/
    /*get_mempolicy(&cur_pol, NULL, 0, 0, 0);*/
    /*struct bitmask* mask = numa_bitmask_alloc(8);*/
    /*numa_bitmask_setall(mask);*/
    /*numa_set_interleave_mask(mask);*/
    /*addr = numa_alloc_interleaved_subset(new_sz, mask);*/
    /*set_mempolicy(cur_pol, mask->maskp, mask->size + 1);*/
    /*numa_bitmask_free(mask);*/
/*#if 0*/
    /*mlock(addr, new_sz);*/
/*#endif*/
/*#if 0*/
    /*// Fault it in*/
    /*memset(addr, 0, new_sz);*/
/*#endif*/

    /*// Get node information*/
    /*uint16_t pagesz = sysconf(_SC_PAGESIZE);*/
    /*size_t num_pages = (sz +  pagesz - 1)/pagesz;*/
    /*uint16_t* node_cache = malloc(sizeof(uint16_t) * num_pages);*/
    /*for(size_t i=0; i<num_pages; i++)*/
    /*{*/
        /*uint16_t node = node_of(addr + (i*pagesz));*/
        /*node_cache[i] = node;*/
        /*if(node == 4)*/
            /*g_sum += 1;*/
    /*}*/
    /*//free(node_cache);*/

    /*return (void*) ((unsigned char*)addr);*/
/*}[>}}}<]*/

/*int coarse_node = 0;*/
/*static void* coarse_allocate(size_t sz)*/
/*{[>{{{<]*/
    /*void* addr = NULL;*/
    /*size_t new_sz = sz;*/

    /*numa_set_bind_policy(1);*/
    /*struct bitmask* mask = numa_bitmask_alloc(8);*/
    /*numa_bitmask_clearall(mask);*/
    /*numa_bitmask_setbit(mask, coarse_node);*/
    /*numa_set_membind(mask);*/
    /*addr = numa_alloc_onnode(new_sz, coarse_node);*/
    /*numa_bitmask_free(mask);*/
    /*numa_set_bind_policy(0);*/
/*#if 0*/
    /*mlock(addr, new_sz);*/
/*#endif*/
/*#if 0*/
    /*// Fault it in*/
    /*memset(addr, 0, new_sz);*/
/*#endif*/

    /*coarse_node++;*/
    /*if(coarse_node == 8)*/
        /*coarse_node = 0;*/

    /*return (void*) ((unsigned char*)addr);*/
/*}[>}}}<]*/

/*void map_init_coarse()*/
/*{[>{{{<]*/
    /*PMSG("Init ... \n");*/
    /*PDBG("Allocating memory ... \n");*/
    /*PMSG("Memory required = %lu MB\n", (buf_sz*sizeof(uint64_t)*num_tasks)/(1024*1024));*/
    /*buffer = malloc (sizeof(uint64_t*) * num_tasks);*/
    /*if(!buffer)*/
        /*PABRT("Cannot allocate memory!\n");*/
    /*for(uint64_t j=0; j<num_tasks; j++)*/
    /*{*/
        /*buffer[j] = coarse_allocate(sizeof(uint64_t) * buf_sz);*/
        /*if(!buffer[j])*/
            /*PABRT("Cannot allocate memory!\n");*/
    /*}*/

/*#ifdef ENABLE_FAULT_IN*/
    /*long time_start = get_usecs();*/
    /*PDBG("Initializing memory ... \n");*/
    /*for(uint64_t i=0; i<num_tasks; i++)*/
        /*for(uint64_t j=0; j<buf_sz; j++)*/
            /*buffer[i][j] = i+1;*/
    /*long time_end = get_usecs();*/
    /*double time = (double)( time_end - time_start) / 1000000;*/
    /*PALWAYS("Coarse init time = %f seconds\n", time);*/
/*#endif*/
/*}[>}}}<]*/

/*void map_init_interleave()*/
/*{[>{{{<]*/
    /*PMSG("Init ... \n");*/
    /*PDBG("Allocating memory ... \n");*/
    /*PMSG("Memory required = %lu MB\n", (buf_sz*sizeof(uint64_t)*num_tasks)/(1024*1024));*/
    /*buffer = malloc (sizeof(uint64_t*) * num_tasks);*/
    /*if(!buffer)*/
        /*PABRT("Cannot allocate memory!\n");*/
    /*for(uint64_t j=0; j<num_tasks; j++)*/
    /*{*/
        /*buffer[j] = interleaved_allocate(sizeof(uint64_t) * buf_sz);*/
        /*if(!buffer[j])*/
            /*PABRT("Cannot allocate memory!\n");*/
    /*}*/

/*#ifdef ENABLE_FAULT_IN*/
    /*long time_start = get_usecs();*/
    /*PDBG("Initializing memory ... \n");*/
    /*for(uint64_t i=0; i<num_tasks; i++)*/
        /*for(uint64_t j=0; j<buf_sz; j++)*/
            /*buffer[i][j] = i+1;*/
    /*long time_end = get_usecs();*/
    /*double time = (double)( time_end - time_start) / 1000000;*/
    /*PALWAYS("Interleave init time = %f seconds\n", time);*/
/*#endif*/
/*}[>}}}<]*/

void map_init()
{/*{{{*/
    PMSG("Init ... \n");
    PDBG("Allocating memory ... \n");
    PMSG("Memory required = %lu MB\n", (buf_sz*sizeof(uint64_t)*num_tasks)/(1024*1024));
    buffer = malloc (sizeof(uint64_t*) * num_tasks);
    if(!buffer)
        PABRT("Cannot allocate memory!\n");
    for(uint64_t j=0; j<num_tasks; j++)
    {
        buffer[j] = mir_mem_pol_allocate(sizeof(uint64_t) * buf_sz);
        if(!buffer[j])
            PABRT("Cannot allocate memory!\n");
    }

#ifdef ENABLE_FAULT_IN
    {
        long time_start = get_usecs();
        PDBG("Initializing memory ... \n");
        for(uint64_t i=0; i<num_tasks; i++)
            for(uint64_t j=0; j<buf_sz; j++)
                buffer[i][j] = i+1;
        long time_end = get_usecs();
        double time = (double)(time_end - time_start) / 1000000;
        PALWAYS("Init time = %f seconds\n", time);
    }
#endif
}/*}}}*/

void map(uint64_t* in, uint64_t* out)
{/*{{{*/
    size_t sum = 0;

// This is quite stupid!
// They will all leave the cache without hitting
#ifdef ENABLE_PREFETCH
PREFETCH_T0(in, buf_sz*sizeof(uint64_t));
#endif

    for(uint64_t j=0; j<LOOP_CNT; j++)
        for(uint64_t i=0; i<buf_sz; i++)
        {
            out[i] = OPR_SCALE * in[i];
            sum += (int)sqrt((double)(out[i])); 
        }

    __sync_fetch_and_add(&g_sum, sum);
    //mir_sleep_ms(SLEEP_MS);
}/*}}}*/

struct map_wrapper_arg_t 
{/*{{{*/
    uint64_t* in;
    uint64_t* out;
};/*}}}*/

void map_wrapper(void* arg)
{/*{{{*/
    struct map_wrapper_arg_t* warg = (struct map_wrapper_arg_t*)(arg);
    map(warg->in, warg->out);
}/*}}}*/

void for_task(uint64_t start, uint64_t end, struct mir_twc_t* twc)
{/*{{{*/
    for(uint64_t j=start; j<=end; j++)
    {/*{{{*/
        // Create task
        {
            // Data env
            struct map_wrapper_arg_t arg;
            arg.in = buffer[j];
            arg.out = buffer[j];
            PDBG("Task %lu reads buf %p and writes buf %p\n", j, buffer[j], buffer[j]);

            // Data footprint
            struct mir_data_footprint_t footprint;
            footprint.base = (void*) buffer[j];
            footprint.start = 0;
            footprint.end = buf_sz - 1;
            footprint.row_sz = 1;
            footprint.type = sizeof(uint64_t);
            footprint.data_access = MIR_DATA_ACCESS_READ;
            footprint.part_of = NULL;

            struct mir_task_t* task = mir_task_create((mir_tfunc_t) map_wrapper, &arg, sizeof(struct map_wrapper_arg_t), twc, 1, &footprint, NULL);
        }
    }/*}}}*/
}/*}}}*/

struct for_task_wrapper_arg_t
{/*{{{*/
    uint64_t start;
    uint64_t end;
    struct mir_twc_t* twc;
};/*}}}*/

void for_task_wrapper(void* arg)
{/*{{{*/
    struct for_task_wrapper_arg_t* warg = (struct for_task_wrapper_arg_t*) arg;
    for_task(warg->start, warg->end, warg->twc);
}/*}}}*/

void map_par()
{/*{{{*/
    PMSG("Parallel exec ... \n");

    struct mir_twc_t* twc = mir_twc_create();

    for(int r=0; r<REUSE_CNT; r++)
    {
        // Split the task creation load among workers
        // Same action as worksharing omp for
        uint32_t num_workers = mir_get_num_threads();
        uint64_t num_iter = num_tasks / num_workers;
        uint64_t num_tail_iter = num_tasks % num_workers;
        if(num_iter > 0)
        {
            // Create prologue tasks
            for(uint32_t k=0; k<num_workers; k++)
            {
                uint64_t start = k * num_iter;
                uint64_t end = start + num_iter - 1;
                /*{*/
                    /*struct for_task_wrapper_arg_t arg;*/
                    /*arg.start = start;*/
                    /*arg.end = end;*/
                    /*arg.twc = twc;*/

                    /*struct mir_task_t* task = mir_task_create((mir_tfunc_t) for_task_wrapper, &arg, sizeof(struct for_task_wrapper_arg_t), twc, 0, NULL, NULL);*/
                /*}*/
                for_task(start, end, twc);
            }
        }
        if(num_tail_iter > 0)
        {
            // Create epilogue task
            uint64_t start = num_workers * num_iter;
            uint64_t end = start + num_tail_iter - 1;
            /*{*/
                /*struct for_task_wrapper_arg_t arg;*/
                /*arg.start = start;*/
                /*arg.end = end;*/
                /*arg.twc = twc;*/

                /*struct mir_task_t* task = mir_task_create((mir_tfunc_t) for_task_wrapper, &arg, sizeof(struct for_task_wrapper_arg_t), twc, 0, NULL, NULL);*/
            /*}*/
            for_task(start, end, twc);
        }

        mir_twc_wait(twc);
    }
    PDBG(" ... done! \n");
}/*}}}*/

int map_check()
{/*{{{*/
    PMSG("Check ... \n");
    return TEST_NOT_PERFORMED;
}/*}}}*/

void map_deinit()
{/*{{{*/
    PMSG("Deinit ... \n");
    for(uint64_t j=0; j<num_tasks; j++)
        //free(buffer[j]/*, sizeof(uint64_t) * buf_sz*/);
        mir_mem_pol_release(buffer[j], sizeof(uint64_t) * buf_sz);
    free(buffer/*, sizeof(uint64_t*) * num_tasks*/);
}/*}}}*/

int main(int argc, char *argv[])
{/*{{{*/
    if (argc > 4)
        PABRT("Usage: %s num_tasks buf_size_K\n", argv[0]);

    // Init the runtime
    mir_create();

    if(argc == 2)
        num_tasks = atoi(argv[1]);

    if(argc == 3)
    {
        num_tasks = atoi(argv[1]);
        buf_sz = atoi(argv[2]) * 1024;
    }

    /*int coarse_or_fine = 0;
    if(argc == 4)
    {
        num_tasks = atoi(argv[1]);
        buf_sz = atoi(argv[2]) * 1024;
        coarse_or_fine = atoi(argv[3]);
    }

    if(coarse_or_fine == 1)
        map_init_coarse();
    else
        map_init_interleave();*/
    map_init();

#ifdef SHOW_NUMA_STATS
    char cmd[256];
    int pid = getpid();
    sprintf(cmd, "numastat %d > numa_stats-%d\n", pid, pid);
    system(cmd);
    sprintf(cmd, "cp /proc/%d/maps maps-%d\n", pid, pid);
    system(cmd);
#endif

    long par_time_start = get_usecs();
    map_par();
    long par_time_end = get_usecs();
    double par_time = (double)( par_time_end - par_time_start) / 1000000;

    int check = TEST_NOT_PERFORMED;
#ifdef CHECK_RESULT 
    check = map_check();
#endif

    map_deinit();

    PMSG("Sum = %lu\n", g_sum);
    PMSG("%s(%d,%d),check=%d in %s,time=%f secs\n", argv[0], num_tasks, buf_sz, check, TEST_ENUM_STRING, par_time);
    PALWAYS("%fs\n", par_time);

    // Pull down the runtime
    mir_destroy();

    return 0;
}/*}}}*/
