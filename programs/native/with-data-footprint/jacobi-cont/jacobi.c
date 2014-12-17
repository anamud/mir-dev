#include <stdio.h>
#include <stdint.h>
#include <sys/time.h>
#include <math.h>
#include <assert.h>
#include <stdlib.h>
#include <unistd.h>

#include "mir_lib_int.h"
#include "helper.h"

// Defines
//#define SHOW_NUMA_STATS

// Constants
const float HEAT_LEFT = 100.0;
const float HEAT_RIGHT = 100.0;
const float HEAT_TOP = 50.0;
const float HEAT_BOT = 50.0;
const float HEAT_INTERIOR = 1.0;
const int HEAT_INTERIOR_SCALE  = 1;
const char* init_plate_file = "./jacobi-init.plate";
const char* final_plate_file = "./jacobi-final.plate";
const int max_iters = 1;

// Globals
int worksharing_chunk_size = 8;
int DIM;
int BS = 256;
int NB = 32;
float** plate = NULL;
float** oldplate = NULL;

// Macros
#define BREF(ARR,I,J) ARR[((I)/(BS))*NB + ((J)/(BS))][((I)%(NB))*(NB) + ((J)%(NB))]

long get_usecs(void)
{/*{{{*/
    struct timeval t;
    gettimeofday(&t,NULL);
    return t.tv_sec*1000000 + t.tv_usec;
}/*}}}*/

void jacobi_point(unsigned int row, unsigned int col, bool print)
{/*{{{*/
    if(row>0 && row<DIM-1 && col>0 && col<DIM-1)
    {
        int i = row;
        int j = col;
        // Central difference approximation
        BREF(plate,i,j) = (BREF(oldplate,i+1,j) + BREF(oldplate,i-1,j)
                + BREF(oldplate,i,j+1) + BREF(oldplate,i,j-1)) / 4;
        if(print == true)
        {
            PMSG("Task reads these addresses:\n");
            PMSG("%p\n", &BREF(oldplate,i+1,j));
            PMSG("%p\n", &BREF(oldplate,i-1,j));
            PMSG("%p\n", &BREF(oldplate,i,j+1));
            PMSG("%p\n", &BREF(oldplate,i,j-1));
            PMSG("Task writes these addresses:\n");
            PMSG("%p\n", &BREF(plate,i,j));
        }
    }
}/*}}}*/

void jacobi_block(unsigned int row, unsigned int col, bool print)
{/*{{{*/
    for(int i=0; i<BS; i++)
        for(int j=0; j<BS; j++)
        {
            int prow = (col*BS) + j;
            int pcol = (row*BS) + i;
            jacobi_point(prow, pcol, print);
            //jacobi_point(pcol, prow, print);
        }
}/*}}}*/

struct jacobi_block_wrapper_arg_t
{/*{{{*/
    unsigned int row;
    unsigned int col;
    bool print;
};/*}}}*/

void jacobi_block_wrapper(void* arg)
{/*{{{*/
    struct jacobi_block_wrapper_arg_t* warg = (struct jacobi_block_wrapper_arg_t*)(arg);
    jacobi_block(warg->row, warg->col, warg->print);
}/*}}}*/

enum BTYPES
{/*{{{*/
    TOP=0,
    BOTTOM,
    LEFT,
    RIGHT,
    MIDDLE,
    TOPRIGHT,
    TOPLEFT,
    BOTTOMRIGHT,
    BOTTOMLEFT
};/*}}}*/
typedef enum BTYPES BTYPE;

static inline BTYPE get_block_type(int row, int col)
{/*{{{*/
    if(row == 0 && col == 0)
        return TOPLEFT;
    if(row == 0 && col == NB-1)
        return TOPRIGHT;
    if(row == NB-1 && col == 0)
        return BOTTOMLEFT;
    if(row == NB-1 && col == NB-1)
        return BOTTOMRIGHT;
    if(row < NB-1 && row > 0 && col > 0 && col < NB-1)
        return MIDDLE;
    if(col == NB-1)
        return RIGHT;
    if(col == 0)
        return LEFT;
    if(row == 0)
        return TOP;
    if(row == NB-1)
        return BOTTOM;
    return MIDDLE;
}/*}}}*/

static inline int get_num_footprints(int row, int col)
{/*{{{*/
    BTYPE b = get_block_type(row, col);
    switch(b)
    {
        case TOP:
        case BOTTOM:
        case LEFT:
        case RIGHT:
            return 3 + 2;
        case TOPRIGHT:
        case TOPLEFT:
        case BOTTOMRIGHT:
        case BOTTOMLEFT:
            return 2 + 2;
        default:
            return 4 + 2;
    }
}/*}}}*/

static inline void fill_footprints(int row, int col, struct mir_data_footprint_t* footprints)
{/*{{{*/
    BTYPE b = get_block_type(row, col);
    int num_footprints = get_num_footprints(row, col);
    int i = 0;
    int block_num;
    switch(b)
    {/*{{{*/
        case TOP:
            block_num = (row+1)*NB + (col);
            assert(block_num >= 0);
            footprints[i].base = (void*) oldplate[(row+1)*NB + (col)];
            footprints[i].start = 0;
            footprints[i].end = (BS*BS)-1;
            footprints[i].row_sz = 1;
            footprints[i].type = sizeof(float);
            footprints[i].data_access = MIR_DATA_ACCESS_READ;
            footprints[i].part_of = footprints[i].base;
            i++;
            block_num = (row)*NB + (col+1);
            assert(block_num >= 0);
            footprints[i].base = (void*) oldplate[(row)*NB + (col+1)];
            footprints[i].start = 0;
            footprints[i].end = (BS*BS)-1;
            footprints[i].row_sz = 1;
            footprints[i].type = sizeof(float);
            footprints[i].data_access = MIR_DATA_ACCESS_READ;
            footprints[i].part_of = footprints[i].base;
            i++;
            block_num = (row)*NB + (col-1);
            assert(block_num >= 0);
            footprints[i].base = (void*) oldplate[(row)*NB + (col-1)];
            footprints[i].start = 0;
            footprints[i].end = (BS*BS)-1;
            footprints[i].row_sz = 1;
            footprints[i].type = sizeof(float);
            footprints[i].data_access = MIR_DATA_ACCESS_READ;
            footprints[i].part_of = footprints[i].base;
            i++;
            break;
        case BOTTOM:
            block_num = (row-1)*NB + (col);
            assert(block_num >= 0);
            footprints[i].base = (void*) oldplate[(row-1)*NB + (col)];
            footprints[i].start = 0;
            footprints[i].end = (BS*BS)-1;
            footprints[i].row_sz = 1;
            footprints[i].type = sizeof(float);
            footprints[i].data_access = MIR_DATA_ACCESS_READ;
            footprints[i].part_of = footprints[i].base;
            i++;
            block_num = (row)*NB + (col+1);
            assert(block_num >= 0);
            footprints[i].base = (void*) oldplate[(row)*NB + (col+1)];
            footprints[i].start = 0;
            footprints[i].end = (BS*BS)-1;
            footprints[i].row_sz = 1;
            footprints[i].type = sizeof(float);
            footprints[i].data_access = MIR_DATA_ACCESS_READ;
            footprints[i].part_of = footprints[i].base;
            i++;
            block_num = (row)*NB + (col-1);
            assert(block_num >= 0);
            footprints[i].base = (void*) oldplate[(row)*NB + (col-1)];
            footprints[i].start = 0;
            footprints[i].end = (BS*BS)-1;
            footprints[i].row_sz = 1;
            footprints[i].type = sizeof(float);
            footprints[i].data_access = MIR_DATA_ACCESS_READ;
            footprints[i].part_of = footprints[i].base;
            i++;
            break;
        case LEFT:
            block_num = (row-1)*NB + (col);
            assert(block_num >= 0);
            footprints[i].base = (void*) oldplate[(row-1)*NB + (col)];
            footprints[i].start = 0;
            footprints[i].end = (BS*BS)-1;
            footprints[i].row_sz = 1;
            footprints[i].type = sizeof(float);
            footprints[i].data_access = MIR_DATA_ACCESS_READ;
            footprints[i].part_of = footprints[i].base;
            i++;
            block_num = (row+1)*NB + (col);
            assert(block_num >= 0);
            footprints[i].base = (void*) oldplate[(row+1)*NB + (col)];
            footprints[i].start = 0;
            footprints[i].end = (BS*BS)-1;
            footprints[i].row_sz = 1;
            footprints[i].type = sizeof(float);
            footprints[i].data_access = MIR_DATA_ACCESS_READ;
            footprints[i].part_of = footprints[i].base;
            i++;
            block_num = (row)*NB + (col+1);
            assert(block_num >= 0);
            footprints[i].base = (void*) oldplate[(row)*NB + (col+1)];
            footprints[i].start = 0;
            footprints[i].end = (BS*BS)-1;
            footprints[i].row_sz = 1;
            footprints[i].type = sizeof(float);
            footprints[i].data_access = MIR_DATA_ACCESS_READ;
            footprints[i].part_of = footprints[i].base;
            i++;
            break;
        case RIGHT:
            block_num = (row-1)*NB + (col);
            assert(block_num >= 0);
            footprints[i].base = (void*) oldplate[(row-1)*NB + (col)];
            footprints[i].start = 0;
            footprints[i].end = (BS*BS)-1;
            footprints[i].row_sz = 1;
            footprints[i].type = sizeof(float);
            footprints[i].data_access = MIR_DATA_ACCESS_READ;
            footprints[i].part_of = footprints[i].base;
            i++;
            block_num = (row+1)*NB + (col);
            assert(block_num >= 0);
            footprints[i].base = (void*) oldplate[(row+1)*NB + (col)];
            footprints[i].start = 0;
            footprints[i].end = (BS*BS)-1;
            footprints[i].row_sz = 1;
            footprints[i].type = sizeof(float);
            footprints[i].data_access = MIR_DATA_ACCESS_READ;
            footprints[i].part_of = footprints[i].base;
            i++;
            block_num = (row)*NB + (col-1);
            assert(block_num >= 0);
            footprints[i].base = (void*) oldplate[(row)*NB + (col-1)];
            footprints[i].start = 0;
            footprints[i].end = (BS*BS)-1;
            footprints[i].row_sz = 1;
            footprints[i].type = sizeof(float);
            footprints[i].data_access = MIR_DATA_ACCESS_READ;
            footprints[i].part_of = footprints[i].base;
            i++;
            break;
        case TOPRIGHT:
            block_num = (row+1)*NB + (col);
            assert(block_num >= 0);
            footprints[i].base = (void*) oldplate[(row+1)*NB + (col)];
            footprints[i].start = 0;
            footprints[i].end = (BS*BS)-1;
            footprints[i].row_sz = 1;
            footprints[i].type = sizeof(float);
            footprints[i].data_access = MIR_DATA_ACCESS_READ;
            footprints[i].part_of = footprints[i].base;
            i++;
            block_num = (row)*NB + (col-1);
            assert(block_num >= 0);
            footprints[i].base = (void*) oldplate[(row)*NB + (col-1)];
            footprints[i].start = 0;
            footprints[i].end = (BS*BS)-1;
            footprints[i].row_sz = 1;
            footprints[i].type = sizeof(float);
            footprints[i].data_access = MIR_DATA_ACCESS_READ;
            footprints[i].part_of = footprints[i].base;
            i++;
            break;
        case TOPLEFT:
            block_num = (row)*NB + (col+1);
            assert(block_num >= 0);
            footprints[i].base = (void*) oldplate[(row)*NB + (col+1)];
            footprints[i].start = 0;
            footprints[i].end = (BS*BS)-1;
            footprints[i].row_sz = 1;
            footprints[i].type = sizeof(float);
            footprints[i].data_access = MIR_DATA_ACCESS_READ;
            footprints[i].part_of = footprints[i].base;
            i++;
            block_num = (row+1)*NB + (col);
            assert(block_num >= 0);
            footprints[i].base = (void*) oldplate[(row+1)*NB + (col)];
            footprints[i].start = 0;
            footprints[i].end = (BS*BS)-1;
            footprints[i].row_sz = 1;
            footprints[i].type = sizeof(float);
            footprints[i].data_access = MIR_DATA_ACCESS_READ;
            footprints[i].part_of = footprints[i].base;
            i++;
            break;
        case BOTTOMRIGHT:
            block_num = (row)*NB + (col-1);
            assert(block_num >= 0);
            footprints[i].base = (void*) oldplate[(row)*NB + (col-1)];
            footprints[i].start = 0;
            footprints[i].end = (BS*BS)-1;
            footprints[i].row_sz = 1;
            footprints[i].type = sizeof(float);
            footprints[i].data_access = MIR_DATA_ACCESS_READ;
            footprints[i].part_of = footprints[i].base;
            i++;
            block_num = (row-1)*NB + (col);
            assert(block_num >= 0);
            footprints[i].base = (void*) oldplate[(row-1)*NB + (col)];
            footprints[i].start = 0;
            footprints[i].end = (BS*BS)-1;
            footprints[i].row_sz = 1;
            footprints[i].type = sizeof(float);
            footprints[i].data_access = MIR_DATA_ACCESS_READ;
            footprints[i].part_of = footprints[i].base;
            i++;
            break;
        case BOTTOMLEFT:
            block_num = (row)*NB + (col-1);
            assert(block_num >= 0);
            footprints[i].base = (void*) oldplate[(row)*NB + (col+1)];
            footprints[i].start = 0;
            footprints[i].end = (BS*BS)-1;
            footprints[i].row_sz = 1;
            footprints[i].type = sizeof(float);
            footprints[i].data_access = MIR_DATA_ACCESS_READ;
            footprints[i].part_of = footprints[i].base;
            i++;
            block_num = (row-1)*NB + (col);
            assert(block_num >= 0);
            footprints[i].base = (void*) oldplate[(row-1)*NB + (col)];
            footprints[i].start = 0;
            footprints[i].end = (BS*BS)-1;
            footprints[i].row_sz = 1;
            footprints[i].type = sizeof(float);
            footprints[i].data_access = MIR_DATA_ACCESS_READ;
            footprints[i].part_of = footprints[i].base;
            i++;
            break;
        default:
            block_num = (row-1)*NB + (col);
            assert(block_num >= 0);
            footprints[i].base = (void*) oldplate[(row-1)*NB + (col)];
            footprints[i].start = 0;
            footprints[i].end = (BS*BS)-1;
            footprints[i].row_sz = 1;
            footprints[i].type = sizeof(float);
            footprints[i].data_access = MIR_DATA_ACCESS_READ;
            footprints[i].part_of = footprints[i].base;
            i++;
            block_num = (row+1)*NB + (col);
            assert(block_num >= 0);
            footprints[i].base = (void*) oldplate[(row+1)*NB + (col)];
            footprints[i].start = 0;
            footprints[i].end = (BS*BS)-1;
            footprints[i].row_sz = 1;
            footprints[i].type = sizeof(float);
            footprints[i].data_access = MIR_DATA_ACCESS_READ;
            footprints[i].part_of = footprints[i].base;
            i++;
            block_num = (row)*NB + (col+1);
            assert(block_num >= 0);
            footprints[i].base = (void*) oldplate[(row)*NB + (col+1)];
            footprints[i].start = 0;
            footprints[i].end = (BS*BS)-1;
            footprints[i].row_sz = 1;
            footprints[i].type = sizeof(float);
            footprints[i].data_access = MIR_DATA_ACCESS_READ;
            footprints[i].part_of = footprints[i].base;
            i++;
            block_num = (row)*NB + (col-1);
            assert(block_num >= 0);
            footprints[i].base = (void*) oldplate[(row)*NB + (col-1)];
            footprints[i].start = 0;
            footprints[i].end = (BS*BS)-1;
            footprints[i].row_sz = 1;
            footprints[i].type = sizeof(float);
            footprints[i].data_access = MIR_DATA_ACCESS_READ;
            footprints[i].part_of = footprints[i].base;
            i++;
            break;
    }/*}}}*/

    block_num = (row)*NB + (col);
    assert(block_num >= 0);
    footprints[i].base = (void*) oldplate[(row)*NB + (col)];
    footprints[i].start = 0;
    footprints[i].end = (BS*BS)-1;
    footprints[i].row_sz = 1;
    footprints[i].type = sizeof(float);
    footprints[i].data_access = MIR_DATA_ACCESS_READ;
    footprints[i].part_of = footprints[i].base;
    i++;

    block_num = (row)*NB + (col);
    assert(block_num >= 0);
    footprints[i].base = (void*) plate[(row)*NB + (col)];
    footprints[i].start = 0;
    footprints[i].end = (BS*BS)-1;
    footprints[i].row_sz = 1;
    footprints[i].type = sizeof(float);
    footprints[i].data_access = MIR_DATA_ACCESS_WRITE;
    footprints[i].part_of = footprints[i].base;
    i++;

    assert(i == num_footprints);
}/*}}}*/

static void print_footprints(struct mir_data_footprint_t* footprints, int num_footprints)
{/*{{{*/
    PMSG("Footprint composed of these addresses:\n");
    for(int i=0; i<num_footprints; i++)
    {
        struct mir_data_footprint_t* footprint = &footprints[i];
        PMSG("%p[%lu-%lu]\n", footprint->base, footprint->start, footprint->end);
    }
}/*}}}*/

void jacobi_par()
{/*{{{*/
    for(int iters=0;iters<max_iters; iters++)
    {
        for(int i=0; i<NB; i++) 
        {
            for(int j=0; j<NB; j++) 
            {
//#pragma omp task 
            //jacobi_block(i, j, oldplate, plate);

            // Data env
            struct jacobi_block_wrapper_arg_t arg;
            arg.row = i;
            arg.col = j;
            /*if(j==0 && i==0)*/
                /*arg.print = true;*/
            /*else*/
                /*arg.print = false;*/
            arg.print = false;

            // Data footprint
            int num_footprints = get_num_footprints(i,j);
            struct mir_data_footprint_t footprints[num_footprints];
            fill_footprints(i,j,footprints);
            /*if(j==0 && i==0)*/
                /*print_footprints(footprints, num_footprints);*/

            mir_task_create((mir_tfunc_t) jacobi_block_wrapper, &arg, sizeof(struct jacobi_block_wrapper_arg_t), num_footprints, footprints, "jacobi_block_wrapper");
            }
        }
//#pragma omp taskwait
        mir_task_wait();

        // Flip plates
        float** temp = plate;
        plate = oldplate;
        oldplate = temp;
        //}

        // Print error in this iteration
        PMSG("Iteration %d done!\n", iters);
    } 
}/*}}}*/

void for_task(int start, int end)
{/*{{{*/
    for(int i=start; i<=end; i++)
    {
        for(int j=0; j<NB; j++) 
        {
//#pragma omp task 
        //jacobi_block(i, j, oldplate, plate);

        // Data env
        struct jacobi_block_wrapper_arg_t arg;
        arg.row = i;
        arg.col = j;
        arg.print = false;

        int num_footprints = get_num_footprints(i,j);
        struct mir_data_footprint_t footprints[num_footprints];
        fill_footprints(i,j,footprints);

        mir_task_create((mir_tfunc_t) jacobi_block_wrapper, &arg, sizeof(struct jacobi_block_wrapper_arg_t), num_footprints, footprints, "jacobi_block_wrapper");
        }
    }
}/*}}}*/

struct for_task_wrapper_arg_t
{/*{{{*/
    uint64_t start;
    uint64_t end;
};/*}}}*/

void for_task_wrapper(void* arg)
{/*{{{*/
    struct for_task_wrapper_arg_t* warg = (struct for_task_wrapper_arg_t*) arg;
    for_task(warg->start, warg->end);
}/*}}}*/

void jacobi_par_worksharing()
{/*{{{*/
    for(int iters=0;iters<max_iters; iters++)
    {
        // Split the task creation load among workers
        // Same action as worksharing omp for
        //uint32_t num_workers = mir_get_num_threads();
        uint32_t num_workers = worksharing_chunk_size;
        uint64_t num_iter = NB / num_workers;
        uint64_t num_tail_iter = NB % num_workers;
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
                    for_task(arg.start , arg.end);
                }
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
                for_task(arg.start , arg.end);
            }
        }
//#pragma omp taskwait
        mir_task_wait();

        // Flip plates
        float** temp = plate;
        plate = oldplate;
        oldplate = temp;
        //}
        //}

        // Print error in this iteration
        PMSG("Iteration %d done!\n", iters);
    } 
}/*}}}*/

void jacobi_allocate()
{/*{{{*/
    plate = (float**) malloc (sizeof(float*) * NB * NB);
    oldplate = (float**) malloc (sizeof(float*) * NB * NB);
    if(plate == NULL || oldplate == NULL) 
        PABRT("Could not allocate memory for plates\n");
    for(int i=0; i<NB*NB; i++)
    {
        if(i%NB == 0)
            mir_mem_pol_reset();
        plate[i] = (float*) mir_mem_pol_allocate(sizeof(float) * BS * BS);
        if(plate[i] == NULL) 
            PABRT("Could not allocate memory for plates\n");
    }
    for(int i=0; i<NB*NB; i++)
    {
        if(i%NB == 0)
            mir_mem_pol_reset();
        oldplate[i] = (float*) mir_mem_pol_allocate(sizeof(float) * BS * BS);
        if(oldplate[i] == NULL) 
            PABRT("Could not allocate memory for plates\n");
    }
}/*}}}*/

void jacobi_wave_allocate()
{/*{{{*/
    plate = (float**) malloc (sizeof(float*) * NB * NB);
    oldplate = (float**) malloc (sizeof(float*) * NB * NB);
    if(plate == NULL || oldplate == NULL) 
        PABRT("Could not allocate memory for plates\n");
    for(int i=0; i<NB; i++)
    {
        for(int j=0; j<NB; j++)
        {
            if(i%2 == 0 && j == 0)
                mir_mem_pol_reset();
            if(i%2 == 1 && j == 1)
                mir_mem_pol_reset();
            plate[i*NB+j] = (float*) mir_mem_pol_allocate(sizeof(float) * BS * BS);
            if(plate[i*NB+j] == NULL) 
                PABRT("Could not allocate memory for plates\n");
            /*else*/
                /*PMSG("%p[%d-%d] = plate[%d]\n", plate[i*NB+j] , 0, BS*BS-1, i*NB+j);*/
        }
    }
    for(int i=0; i<NB; i++)
    {
        for(int j=0; j<NB; j++)
        {
            if(i%2 == 0 && j == 0)
                mir_mem_pol_reset();
            if(i%2 == 1 && j == 1)
                mir_mem_pol_reset();
            oldplate[i*NB+j] = (float*) mir_mem_pol_allocate(sizeof(float) * BS * BS);
            if(oldplate[i*NB+j] == NULL) 
                PABRT("Could not allocate memory for plates\n");
            /*else*/
                /*PMSG("%p[%d-%d] = oldplate[%d]\n", oldplate[i*NB+j], 0, BS*BS-1, i*NB+j);*/
        }
    }
}/*}}}*/

int main(int argc, char* argv[])
{/*{{{*/
    PMSG("Getting args ...\n");
    /*{{{*/
    if(argc > 3)
        PABRT("Usage: %s NB BS\n", argv[0]);

    if(argc == 2)
        NB = atoi(argv[1]);

    if(argc == 3)
    {
        NB = atoi(argv[1]);
        BS = atoi(argv[2]);
    }

    DIM = NB*BS;
    /*}}}*/

    // Init the runtime
    mir_create();

    PMSG("Allocating plate ...\n");
    /*{{{*/
    jacobi_allocate();
    //jacobi_wave_allocate();
    /*}}}*/

    PMSG("Setting init conditions of plate ...\n");
    /*{{{*/
#if 1
    long tsi, tei;
    tsi = get_usecs();
    // Initialize plate temperature
    for(int i=0; i<DIM; i++)
        for(int j=0; j<DIM; j++)
        {
            BREF(plate,i,j) = (HEAT_INTERIOR * HEAT_INTERIOR_SCALE);
            BREF(oldplate,i,j) = (HEAT_INTERIOR * HEAT_INTERIOR_SCALE);
        }

    // Boundary contions
    for(int i=0; i<DIM; i++)
    {
        BREF(plate,i,0) = HEAT_LEFT;
        BREF(plate,i,DIM-1) = HEAT_RIGHT;
        BREF(oldplate,i,0) = HEAT_LEFT;
        BREF(oldplate,i,DIM-1) = HEAT_RIGHT;
    }
    for(int i=1; i<DIM-1; i++)
    {
        BREF(plate,0,i) = HEAT_TOP;
        BREF(plate,DIM-1,i) = HEAT_BOT;
        BREF(oldplate,0,i) = HEAT_TOP;
        BREF(oldplate,DIM-1,i) = HEAT_BOT;
    }
    tei = get_usecs();
    PMSG("Init time = %f secons\n", (double) ((tei - tsi))/1000000);
#endif
    /*}}}*/

#ifdef CHECK_RESULT
    PMSG("Writing init plate to file %s ...\n", init_plate_file);
    /*{{{*/
    // Write init plate
    FILE* fp = fopen(init_plate_file, "w");
    if(fp) 
    {
        fprintf(fp, "%d\t%d\n", DIM, DIM);
        for(int i=0; i<DIM; i++) 
        {
            for(int j=0; j<DIM; j++) 
                fprintf(fp, "%f\t", BREF(plate,i,j));
            fprintf(fp, "\n");
        }
    }
    fclose(fp);
    /*}}}*/
#endif

#ifdef SHOW_NUMA_STATS
    char cmd[256];
    sprintf(cmd, "~/nmstat %d\n", getpid());
    system(cmd);
    /*printf("Press ENTER key to continue ...");*/
    /*getchar();*/
#endif

    PMSG("Solving plate ...!\n");
    // Iterate and solve/*{{{*/
    long ts, te;
    ts = get_usecs();
    //jacobi_par();
    jacobi_par_worksharing();
    te = get_usecs();/*}}}*/

#ifdef CHECK_RESULT
    PMSG("Writing final plate to file %s ...\n", final_plate_file);
    /*{{{*/
    // Write init plate
    fp = fopen(final_plate_file, "w");
    if(fp) 
    {
        fprintf(fp, "%d\t%d\n", DIM, DIM);
        for(int i=0; i<DIM; i++) 
        {
            for(int j=0; j<DIM; j++) 
                fprintf(fp, "%f\t", BREF(oldplate,i,j));
            fprintf(fp, "\n");
        }
    }
    fclose(fp);
    /*}}}*/
#endif

    PMSG("Releasing plate ...\n");
    /*{{{*/
    // Release memory
    for(int i=0; i<NB*NB; i++)
    {
        mir_mem_pol_release(plate[i], sizeof(float) * BS * BS);
        mir_mem_pol_release(oldplate[i], sizeof(float) * BS * BS);
    }
    free(plate/*, sizeof(float*) * NB * NB*/);
    free(oldplate/*, sizeof(float*) * NB * NB*/);
    /*}}}*/

    // Print stats
    PMSG("Solution using jacobi iteration: DIM = %d, NB=%d, BS=%d, num iterations = %d, time = %f seconds\n", DIM, NB, BS, max_iters, (double) ((te - ts))/1000000);
    PALWAYS("%fs\n", (double) ((te - ts))/1000000);

    // Pull down the runtime
    mir_destroy();

    return 0;
}/*}}}*/
