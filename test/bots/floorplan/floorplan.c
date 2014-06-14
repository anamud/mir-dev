/**********************************************************************************************/
/*  This program is part of the Barcelona OpenMP Tasks Suite                                  */
/*  Copyright (C) 2009 Barcelona Supercomputing Center - Centro Nacional de Supercomputacion  */
/*  Copyright (C) 2009 Universitat Politecnica de Catalunya                                   */
/*                                                                                            */
/*  This program is free software; you can redistribute it and/or modify                      */
/*  it under the terms of the GNU General Public License as published by                      */
/*  the Free Software Foundation; either version 2 of the License, or                         */
/*  (at your option) any later version.                                                       */
/*                                                                                            */
/*  This program is distributed in the hope that it will be useful,                           */
/*  but WITHOUT ANY WARRANTY; without even the implied warranty of                            */
/*  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the                             */
/*  GNU General Public License for more details.                                              */
/*                                                                                            */
/*  You should have received a copy of the GNU General Public License                         */
/*  along with this program; if not, write to the Free Software                               */
/*  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA            */
/**********************************************************************************************/

/* Original code from the Application Kernel Matrix by Cray */

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <alloca.h>
#include <sys/time.h>

#include "mir_public_int.h"
#include "helper.h"

#define ROWS 64
#define COLS 64
#define DMAX 64

typedef int  coor[2];
typedef char ibrd[ROWS][COLS];
typedef char (*pibrd)[COLS];

int cutoff_value = 5;
int solution = -1;
FILE * inputFile;
int number_of_tasks;
int  MIN_AREA;
ibrd BEST_BOARD;
coor MIN_FOOTPRINT;
int N;

struct cell 
{
    int   n;
    coor *alt;
    int   top;
    int   bot;
    int   lhs;
    int   rhs;
    int   left;
    int   above;
    int   next;
};
struct cell * gcells;

struct mir_lock_t add_cell_lock;

#define max(a, b) ((a > b) ? (a) : (b))
#define min(a, b) ((a < b) ? (a) : (b))

#define read_integer(file,var) \
    if ( fscanf(file, "%d", &var) == EOF ) {\
        PMSG(" Bogus input file\n");\
        exit(-1);\
    }

long get_usecs(void)
{/*{{{*/
    struct timeval t;
    gettimeofday(&t, ((void *) 0));
    return t.tv_sec * 1000000 + t.tv_usec;
}/*}}}*/

/* compute all possible locations for nw corner for cell */
/*static*/ int starts(int id, int shape, coor *NWS, struct cell *cells) 
{/*{{{*/
    int i, n, top, bot, lhs, rhs;
    int rows, cols, left, above;

    /* size of cell */
    rows  = cells[id].alt[shape][0];
    cols  = cells[id].alt[shape][1];

    /* the cells to the left and above */
    left  = cells[id].left;
    above = cells[id].above;

    /* if there is a vertical and horizontal dependence */
    if ((left >= 0) && (above >= 0)) {

        top = cells[above].bot + 1;
        lhs = cells[left].rhs + 1;
        bot = top + rows;
        rhs = lhs + cols;

        /* if footprint of cell touches the cells to the left and above */
        if ((top <= cells[left].bot) && (bot >= cells[left].top) &&
                (lhs <= cells[above].rhs) && (rhs >= cells[above].lhs))
        { n = 1; NWS[0][0] = top; NWS[0][1] = lhs;  }
        else { n = 0; }

        /* if there is only a horizontal dependence */
    } else if (left >= 0) {

        /* highest initial row is top of cell to the left - rows */ 
        top = max(cells[left].top - rows + 1, 0);
        /* lowest initial row is bottom of cell to the left */
        bot = min(cells[left].bot, ROWS);
        n   = bot - top + 1;

        for (i = 0; i < n; i++) {
            NWS[i][0] = i + top;
            NWS[i][1] = cells[left].rhs + 1;
        }

    } else {

        /* leftmost initial col is lhs of cell above - cols */
        lhs = max(cells[above].lhs - cols + 1, 0);
        /* rightmost initial col is rhs of cell above */
        rhs = min(cells[above].rhs, COLS);
        n   = rhs - lhs + 1;

        for (i = 0; i < n; i++) {
            NWS[i][0] = cells[above].bot + 1;
            NWS[i][1] = i + lhs;
        }  }

    return (n);
}/*}}}*/

/* lay the cell down on the board in the rectangular space defined
   by the cells top, bottom, left, and right edges. If the cell can
   not be layed down, return 0; else 1.
   */
/*static*/ int lay_down(int id, ibrd board, struct cell *cells)
{/*{{{*/
    int  i, j, top, bot, lhs, rhs;

    top = cells[id].top;
    bot = cells[id].bot;
    lhs = cells[id].lhs;
    rhs = cells[id].rhs;

    for (i = top; i <= bot; i++) {
        for (j = lhs; j <= rhs; j++) {
            if (board[i][j] == 0) board[i][j] = (char)id;
            else                  return(0);
        } }

    return (1);
}/*}}}*/

/*static*/ void read_inputs() 
{/*{{{*/
    int i, j, n;

    read_integer(inputFile,n);
    N = n;

    gcells = (struct cell *) malloc((n + 1) * sizeof(struct cell));

    gcells[0].n     =  0;
    gcells[0].alt   =  0;
    gcells[0].top   =  0;
    gcells[0].bot   =  0;
    gcells[0].lhs   = -1;
    gcells[0].rhs   = -1;
    gcells[0].left  =  0;
    gcells[0].above =  0;
    gcells[0].next  =  0;

    for (i = 1; i < n + 1; i++) {

        read_integer(inputFile, gcells[i].n);
        gcells[i].alt = (coor *) malloc(gcells[i].n * sizeof(coor));

        for (j = 0; j < gcells[i].n; j++) {
            read_integer(inputFile, gcells[i].alt[j][0]);
            read_integer(inputFile, gcells[i].alt[j][1]);
        }

        read_integer(inputFile, gcells[i].left);
        read_integer(inputFile, gcells[i].above);
        read_integer(inputFile, gcells[i].next);
    }

    if (!feof(inputFile)) {
        read_integer(inputFile, solution);
    }
}/*}}}*/

/*static*/ void write_outputs() 
{/*{{{*/
    int i, j;

    PMSG("Minimum area = %d\n\n", MIN_AREA);

    for (i = 0; i < MIN_FOOTPRINT[0]; i++) {
        for (j = 0; j < MIN_FOOTPRINT[1]; j++) {
            if (BEST_BOARD[i][j] == 0) {PMSG(" ");}
            else                       PMSG("%c", 'A' + BEST_BOARD[i][j] - 1);
        } 
        PMSG("\n");
    }  
}/*}}}*/

/*static*/ int add_cell_ser (int id, coor FOOTPRINT, ibrd BOARD, struct cell *CELLS)
{/*{{{*/
    int  i, j, nn, nn2, area;

    ibrd board;
    coor footprint;
    int NWS[DMAX][2];

    nn2 = 0;

    /* for each possible shape */
    for (i = 0; i < CELLS[id].n; i++) 
    {
        /* compute all possible locations for nw corner */
        nn = starts(id, i, NWS, CELLS);
        nn2 += nn;
        /* for all possible locations */
        for (j = 0; j < nn; j++) 
        {
            struct cell *cells = CELLS;
            /* extent of shape */
            cells[id].top = NWS[j][0];
            cells[id].bot = cells[id].top + cells[id].alt[i][0] - 1;
            cells[id].lhs = NWS[j][1];
            cells[id].rhs = cells[id].lhs + cells[id].alt[i][1] - 1;

            memcpy(board, BOARD, sizeof(ibrd));

            /* if the cell cannot be layed down, prune search */
            if (! lay_down(id, board, cells)) 
            {
                PDBG("Chip %d, shape %d does not fit\n", id, i);
                //goto _end;
                continue;
            }

            /* calculate new footprint of board and area of footprint */
            footprint[0] = max(FOOTPRINT[0], cells[id].bot+1);
            footprint[1] = max(FOOTPRINT[1], cells[id].rhs+1);
            area         = footprint[0] * footprint[1];

            /* if last cell */
            if (cells[id].next == 0) 
            {
                /* if area is minimum, update global values */
                if (area < MIN_AREA) 
                {
//#pragma omp critical
mir_lock_set(&(add_cell_lock));
                    if (area < MIN_AREA) 
                    {
                        MIN_AREA         = area;
                        MIN_FOOTPRINT[0] = footprint[0];
                        MIN_FOOTPRINT[1] = footprint[1];
                        memcpy(BEST_BOARD, board, sizeof(ibrd));
                        PDBG("N  %d\n", MIN_AREA);
                    }
mir_lock_unset(&(add_cell_lock));
                }
                /* if area is less than best area */
            } 
            else if (area < MIN_AREA) 
            {
/*#pragma omp atomic*/
                /*nn2 += add_cell_ser(cells[id].next, footprint, board,cells);*/
                int ans = add_cell_ser(cells[id].next, footprint, board,cells);
                __sync_fetch_and_add(&nn2, ans);

                /* if area is greater than or equal to best area, prune search */
            } 
            else 
            {
                PDBG("T  %d, %d\n", area, MIN_AREA);
            }
//_end:; 
        }
    }
    return nn2;
}/*}}}*/

struct  nanos_args_0_t
{
  int *MIN_AREA;
  char (*BEST_BOARD)[64][64];
  int (*MIN_FOOTPRINT)[2];
  int *N;
  int **FOOTPRINT;
  char (**BOARD)[64];
  struct cell **CELLS;
  int *nnc;
  int cutoff_value;
  int id;
  int level;
  int i;
  int j;
  int nn;
  int NWS[64][2];
};

/*static*/ int add_cell(int id, coor FOOTPRINT, ibrd BOARD, struct cell *CELLS,int level);

/*static*/ void smp_ol_add_cell_0_unpacked(int *const MIN_AREA, char (*const BEST_BOARD)[64], int *const MIN_FOOTPRINT, int *const N, int **const FOOTPRINT, char (**const BOARD)[64], struct cell **const CELLS, int *const nnc, int *const cutoff_value, int *const id, int *const level, int *const i, int *const j, int *const nn, int (*const NWS)[2])
{/*{{{*/
  char board[64][64];
  int footprint[2];
  int area;
  {
    {
      struct cell *cells;
      cells = __builtin_alloca(sizeof(struct cell) * ((*N) + 1));
      memcpy(cells, (*CELLS), sizeof(struct cell) * ((*N) + 1));
      cells[(*id)].top = NWS[(*j)][0];
      cells[(*id)].bot = cells[(*id)].top + cells[(*id)].alt[(*i)][0] - 1;
      cells[(*id)].lhs = NWS[(*j)][1];
      cells[(*id)].rhs = cells[(*id)].lhs + cells[(*id)].alt[(*i)][1] - 1;
      memcpy(board, (*BOARD), sizeof(char [64][64]));
      if (!lay_down((*id), board, cells))
        {
          do
            {
              PMSG("Chip %d, shape %d does not fit\n", (*id), (*i));
            }
          while (0);
          goto _end;
        }
      footprint[0] = (*FOOTPRINT)[0] > cells[(*id)].bot + 1 ? (*FOOTPRINT)[0] : cells[(*id)].bot + 1;
      footprint[1] = (*FOOTPRINT)[1] > cells[(*id)].rhs + 1 ? (*FOOTPRINT)[1] : cells[(*id)].rhs + 1;
      area = footprint[0] * footprint[1];
      if (cells[(*id)].next == 0)
        {
          if (area < (*MIN_AREA))
            {
//#pragma omp critical
mir_lock_set(&(add_cell_lock));
              if (area < (*MIN_AREA))
                {
                  (*MIN_AREA) = area;
                  MIN_FOOTPRINT[0] = footprint[0];
                  MIN_FOOTPRINT[1] = footprint[1];
                  memcpy(BEST_BOARD, board, sizeof(char [64][64]));
                  do
                    {
                      PMSG("N  %d\n", (*MIN_AREA));
                    }
                  while (0);
                }
mir_lock_unset(&(add_cell_lock));
            }
        }
      else
        if (area < (*MIN_AREA))
          {
            if ((*level) + 1 < (*cutoff_value))
              {
                int ans = add_cell(cells[(*id)].next, footprint, board, cells, (*level) + 1);
                __sync_fetch_and_add_4((int *) nnc, ans);
              }
            else
              {
                int ans = add_cell_ser(cells[(*id)].next, footprint, board, cells);
                __sync_fetch_and_add_4((int *) nnc, ans);
              }
          }
        else
          {
            do
              {
                PMSG("T  %d, %d\n", area, (*MIN_AREA));
              }
            while (0);
          }
      _end : ;
    }
  }
}/*}}}*/

/*static*/ void smp_ol_add_cell_0(struct nanos_args_0_t *const args)
{/*{{{*/
  {
    smp_ol_add_cell_0_unpacked((*args).MIN_AREA, *(*args).BEST_BOARD, *(*args).MIN_FOOTPRINT, (*args).N, (*args).FOOTPRINT, (*args).BOARD, (*args).CELLS, (*args).nnc, &((*args).cutoff_value), &((*args).id), &((*args).level), &((*args).i), &((*args).j), &((*args).nn), /*&*/((*args).NWS));
  }
}/*}}}*/

/*static*/ int add_cell(int id, coor FOOTPRINT, ibrd BOARD, struct cell *CELLS,int level)
{/*{{{*/
    int  i, j, nn, area, nnc, nnl;

    ibrd board;
    coor footprint;
    int NWS[DMAX][2];

    nnc = nnl = 0;

    mir_lock_create(&add_cell_lock);

    /* for each possible shape */
    for (i = 0; i < CELLS[id].n; i++) 
    {
        /* compute all possible locations for nw corner */
        nn = starts(id, i, NWS, CELLS);
        nnl += nn;
        /* for all possible locations */
        for (j = 0; j < nn; j++) 
        {
/*#pragma omp task private(board, footprint,area) firstprivate(NWS,i,j,id,nn,level,cutoff_value) shared(nnc) shared(FOOTPRINT,BOARD,CELLS,MIN_AREA,MIN_FOOTPRINT,N,BEST_BOARD)*/
            /*{[>{{{<]*/
                /*struct cell *cells;*/

                /*cells = alloca(sizeof(struct cell)*(N+1));*/
                /*memcpy(cells,CELLS,sizeof(struct cell)*(N+1));*/

                /*[> extent of shape <]*/
                /*cells[id].top = NWS[j][0];*/
                /*cells[id].bot = cells[id].top + cells[id].alt[i][0] - 1;*/
                /*cells[id].lhs = NWS[j][1];*/
                /*cells[id].rhs = cells[id].lhs + cells[id].alt[i][1] - 1;*/

                /*memcpy(board, BOARD, sizeof(ibrd));*/

                /*[> if the cell cannot be layed down, prune search <]*/
                /*if (! lay_down(id, board, cells)) {*/
                    /*PDBG("Chip %d, shape %d does not fit\n", id, i);*/
                    /*goto _end;*/
                /*}*/

                /*[> calculate new footprint of board and area of footprint <]*/
                /*footprint[0] = max(FOOTPRINT[0], cells[id].bot+1);*/
                /*footprint[1] = max(FOOTPRINT[1], cells[id].rhs+1);*/
                /*area         = footprint[0] * footprint[1];*/

                /*[> if last cell <]*/
                /*if (cells[id].next == 0) */
                /*{*/
                    /*[> if area is minimum, update global values <]*/
                    /*if (area < MIN_AREA) */
                    /*{*/
/*//#pragma omp critical*/
                        /*if (area < MIN_AREA) */
                        /*{*/
                            /*MIN_AREA         = area;*/
                            /*MIN_FOOTPRINT[0] = footprint[0];*/
                            /*MIN_FOOTPRINT[1] = footprint[1];*/
                            /*memcpy(BEST_BOARD, board, sizeof(ibrd));*/
                            /*PDBG("N  %d\n", MIN_AREA);*/
                        /*}*/
                    /*}*/
                /*} */
                /*else if (area < MIN_AREA) [> if area is less than best area <]*/
                /*{*/
                    /*if(level+1 < cutoff_value ) */
                    /*{*/
/*[>#pragma omp atomic<]*/
                        /*[>nnc += add_cell(cells[id].next, footprint, board,cells,level+1);<]*/
                        /*int ans = add_cell(cells[id].next, footprint, board,cells,level+1);*/
                        /*__sync_fetch_and_add(&nnc, ans);*/

                    /*} */
                    /*else */
                    /*{*/
/*[>#pragma omp atomic<]*/
                        /*[>nnc += add_cell_ser(cells[id].next, footprint, board,cells);<]*/
                        /*int ans = add_cell_ser(cells[id].next, footprint, board,cells);*/
                        /*__sync_fetch_and_add(&nnc, ans);*/
                    /*}*/
                    /*[> if area is greater than or equal to best area, prune search <]*/
                /*} */
                /*else */
                /*{*/
                    /*PDBG("T  %d, %d\n", area, MIN_AREA);*/
                /*}*/
/*_end:;*/
            /*}[>}}}<]*/
            // Create task
            struct nanos_args_0_t imm_args;
            imm_args.MIN_AREA = &MIN_AREA;
            imm_args.BEST_BOARD = &BEST_BOARD;
            imm_args.MIN_FOOTPRINT = &MIN_FOOTPRINT;
            imm_args.N = &N;
            imm_args.FOOTPRINT = &FOOTPRINT;
            imm_args.BOARD = &BOARD;
            imm_args.CELLS = &CELLS;
            imm_args.nnc = &nnc;
            imm_args.cutoff_value = cutoff_value;
            imm_args.id = id;
            imm_args.level = level;
            imm_args.i = i;
            imm_args.j = j;
            imm_args.nn = nn;
            __builtin_memcpy(&imm_args.NWS, &NWS, sizeof(int [64][2]));

            struct mir_task_t* task_0 = mir_task_create((mir_tfunc_t) smp_ol_add_cell_0, (void*) &imm_args, sizeof(struct nanos_args_0_t), 0, NULL, NULL);
        }
    }
//#pragma omp taskwait
    mir_twc_wait();

    return nnc+nnl;
}/*}}}*/

ibrd board;

void floorplan_init (char *filename)
{/*{{{*/
    int i,j;

    inputFile = fopen(filename, "r");

    if(NULL == inputFile) {
        PMSG("Couldn't open %s file for reading\n", filename);
        exit(1);
    }

    /* read input file and initialize global minimum area */
    read_inputs();
    MIN_AREA = ROWS * COLS;

    /* initialize board is empty */
    for (i = 0; i < ROWS; i++)
        for (j = 0; j < COLS; j++) board[i][j] = 0;

}/*}}}*/

void compute_floorplan (void)
{/*{{{*/
    coor footprint;
    /* footprint of initial board is zero */
    footprint[0] = 0;
    footprint[1] = 0;
    PMSG("Computing floorplan ");
/*#pragma omp parallel*/
    /*{*/
/*#pragma omp single*/
        number_of_tasks = add_cell(1, footprint, board, gcells,0);
    /*}*/
    PMSG(" completed!\n");

}/*}}}*/

void floorplan_end (void)
{/*{{{*/
    return;
    /* write results */
    write_outputs();
}/*}}}*/

int floorplan_verify (void)
{/*{{{*/
    if (solution != -1 )
        return MIN_AREA == solution ? TEST_SUCCESSFUL : TEST_UNSUCCESSFUL;
    else
        return TEST_NOT_PERFORMED;
}/*}}}*/

int main(int argc, char *argv[])
{/*{{{*/
    if (argc < 2 || argc > 3)
        PABRT("Usage: %s input-file cutoff\n", argv[0]);

    if(argc > 2)
        cutoff_value = atoi(argv[2]);

    // Init the runtime
    mir_create();

    // Create locks

    floorplan_init(argv[1]);

    long par_time_start = get_usecs();
    compute_floorplan();
    long par_time_end = get_usecs();
    double par_time = (double)( par_time_end - par_time_start) / 1000000;

    floorplan_end();

    int check = TEST_NOT_PERFORMED;
#ifdef CHECK_RESULT
    PDBG("Checking ... \n");
    check = floorplan_verify();
#endif

    PMSG("%s(%s),check=%d in %s,time=%f secs\n", argv[0], argv[1], check, TEST_ENUM_STRING, par_time);
    PALWAYS("%fs\n", par_time);

    // Pull down the runtime
    mir_destroy();

    return 0;
}/*}}}*/
