#include <stdio.h>
#include <stdint.h>
#include <sys/time.h>
#include <math.h>
#include <stdlib.h>
#include <unistd.h>

#include "mir_public_int.h"
#include "helper.h"

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
int DIM = 2048;
int BS = 32;
int NB;
float* plate = NULL;
float* oldplate = NULL;

// Macros
#define RM2D(ARR,NUMCOLS,ROWI,COLI) ARR[(ROWI)*(NUMCOLS) + (COLI)]

long get_usecs(void)
{/*{{{*/
    struct timeval t;
    gettimeofday(&t,NULL);
    return t.tv_sec*1000000 + t.tv_usec;
}/*}}}*/

void jacobi_point(unsigned int row, unsigned int col/*, bool print*/)
{/*{{{*/
    if(row>0 && row<DIM-1 && col>0 && col<DIM-1)
    {
        int i = row;
        int j = col;
        // Central difference approximation
        RM2D(plate,DIM,i,j) = (RM2D(oldplate,DIM,i+1,j) + RM2D(oldplate,DIM,i-1,j) 
                + RM2D(oldplate,DIM,i,j+1) + RM2D(oldplate,DIM,i,j-1)) / 4;
        /*if(print)*/
        /*{*/
            /*PMSG("Task reads these addresses:\n");*/
            /*PMSG("%p\n", &RM2D(oldplate,DIM,i+1,j));*/
            /*PMSG("%p\n", &RM2D(oldplate,DIM,i-1,j));*/
            /*PMSG("%p\n", &RM2D(oldplate,DIM,i,j+1));*/
            /*PMSG("%p\n", &RM2D(oldplate,DIM,i,j-1));*/
            /*PMSG("Task writes these addresses:\n");*/
            /*PMSG("%p\n", &RM2D(plate,DIM,i,j));*/
        /*}*/
    }
}/*}}}*/

void jacobi_block(unsigned int row, unsigned int col/*, bool print*/)
{/*{{{*/
    for(int i=0; i<BS; i++)
        for(int j=0; j<BS; j++)
        {
            int prow = (col*BS) + j;
            int pcol = (row*BS) + i;
            jacobi_point(prow, pcol/*, print*/);
        }
}/*}}}*/

struct jacobi_block_wrapper_arg_t 
{/*{{{*/
    unsigned int row;
    unsigned int col;
    /*bool print;*/
};/*}}}*/

void jacobi_block_wrapper(void* arg)
{/*{{{*/
    struct jacobi_block_wrapper_arg_t* warg = (struct jacobi_block_wrapper_arg_t*)(arg);
    jacobi_block(warg->row, warg->col/*, warg->print*/);
}/*}}}*/

int main(int argc, char* argv[])
{/*{{{*/
    PMSG("Getting args ...\n");
    if(argc > 3)/*{{{*/
        PABRT("Usage: %s DIM BS\n", argv[0]);

    if(argc == 2)
        DIM = atoi(argv[1]);

    if(argc == 3)
    {
        DIM = atoi(argv[1]);
        BS = atoi(argv[2]);
    }

    if(DIM%BS > 0)
        PABRT("BS %d does not divide DIM %d properly\n", BS, DIM);
    int NB = DIM/BS;/*}}}*/

    // Init the runtime
    mir_create();

    PMSG("Allocating plate ...\n");
    plate = (float*) mir_mem_pol_allocate(sizeof(float) * DIM * DIM);/*{{{*/
    oldplate = (float*) mir_mem_pol_allocate(sizeof(float) * DIM * DIM);
    if(plate == NULL || oldplate == NULL) 
        PABRT("Could not allocate memory for plates\n");/*}}}*/
    /*PMSG("Plate: %p\n", plate);*/
    /*PMSG("Oldplate: %p\n", oldplate);*/

    /*char cmd[256];*/
    /*sprintf(cmd, "~/nmstat %d\n", getpid());*/
    /*system(cmd);*/
    /*PMSG("Press ENTER key to continue ...");*/
    /*getchar();*/

    PMSG("Setting init conditions of plate ...\n");
    // Initialize plate temperature/*{{{*/
    for(int i=0; i<DIM; i++)
        for(int j=0; j<DIM; j++) 
        {
            RM2D(plate,DIM,i,j) = (HEAT_INTERIOR * HEAT_INTERIOR_SCALE);
            RM2D(oldplate,DIM,i,j) = (HEAT_INTERIOR * HEAT_INTERIOR_SCALE);
        }

    // Boundary contions
    for(int i=0; i<DIM; i++) 
    {
        RM2D(plate,DIM,i,0) = HEAT_LEFT; 
        RM2D(plate,DIM,i,DIM-1) = HEAT_RIGHT; 
        RM2D(oldplate,DIM,i,0) = HEAT_LEFT; 
        RM2D(oldplate,DIM,i,DIM-1) = HEAT_RIGHT; 
    }
    for(int i=1; i<DIM-1; i++) 
    {
        RM2D(plate,DIM,0,i) = HEAT_TOP;
        RM2D(plate,DIM,DIM-1,i) = HEAT_BOT;
        RM2D(oldplate,DIM,0,i) = HEAT_TOP;
        RM2D(oldplate,DIM,DIM-1,i) = HEAT_BOT;
    }/*}}}*/

#if 0
    PMSG("Writing init plate to file %s ...\n", init_plate_file);
    // Write init plate/*{{{*/
    FILE* fp = fopen(init_plate_file, "w");
    if(fp) 
    {
        fprintf(fp, "%d\t%d\n", DIM, DIM);
        for(int i=0; i<DIM; i++) 
        {
            for(int j=0; j<DIM; j++) 
                fprintf(fp, "%f\t", RM2D(plate,DIM,i,j));
            fprintf(fp, "\n");
        }
    }
    fclose(fp);/*}}}*/
#endif

    PMSG("Solving plate ...!\n");
    // Iterate and solve/*{{{*/
    long ts, te;
    int iters;
    ts = get_usecs();
    int i, j;
    for(iters=0;iters<max_iters; iters++)
    {
        struct mir_twc_t* twc = mir_twc_create();
        for(i=0; i<NB; i++) 
        {
            for(j=0; j<NB; j++) 
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

            // Data footprint
            struct mir_data_footprint_t footprints[2];
            footprints[0].base = (void*) oldplate + (j*BS) + (i*DIM);
            footprints[0].start = 0;
            footprints[0].end = BS-1;
            footprints[0].row_sz = DIM;
            footprints[0].type = sizeof(float);
            footprints[0].data_access = MIR_DATA_ACCESS_READ;
            footprints[0].part_of = oldplate;

            footprints[1].base = (void*) plate + (j*BS) + (i*DIM);
            footprints[1].start = 0;
            footprints[1].end = BS-1;
            footprints[1].row_sz = DIM;
            footprints[1].type = sizeof(float);
            footprints[1].data_access = MIR_DATA_ACCESS_WRITE;
            footprints[1].part_of = plate;

            struct mir_task_t* task = mir_task_create((mir_tfunc_t) jacobi_block_wrapper, &arg, sizeof(struct jacobi_block_wrapper_arg_t), twc, 2, footprints, NULL);
            }
        }
//#pragma omp taskwait
        mir_twc_wait(twc);

        // Flip plates
        float* temp = plate;
        plate = oldplate;
        oldplate = temp;
        //}

        // Print error in this iteration
        PMSG("Iteration %d done!\n", iters);
    } 
    te = get_usecs();/*}}}*/

#if 0
    PMSG("Writing init plate to file %s ...\n", init_plate_file);
    PMSG("Writing final plate to file %s ...\n", final_plate_file);
    // Write solution/*{{{*/
    fp = fopen(final_plate_file, "w");
    if(fp) 
    {
        fprintf(fp, "%d\t%d\n", DIM, DIM);
        for(int i=0; i<DIM; i++) 
        {
            for(int j=0; j<DIM; j++) 
                fprintf(fp, "%f\t", RM2D(plate,DIM,i,j));
            fprintf(fp, "\n");
        }
    }
    fclose(fp);/*}}}*/
#endif

    PMSG("Releasing plate ...\n");
    // Release memory/*{{{*/
    mir_mem_pol_release(plate, sizeof(float) * DIM * DIM);
    mir_mem_pol_release(oldplate, sizeof(float) * DIM * DIM);/*}}}*/

    // Print stats
    PMSG("Solution using jacobi iteration: DIM = %d, BS=%d, num iterations = %d, time = %f seconds\n", DIM, BS, max_iters, (double) ((te - ts))/1000000);

    // Pull down the runtime
    mir_destroy();

    return 0;
}/*}}}*/
