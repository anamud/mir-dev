/**********************************************************************************************/
/*  This program is part of the Barcelona OpenMP Tasks Suite                                  */
/*  Copyright (C) 2009 Barcelona Supercomputing Center - Centro Nacional de Supercomputacion  */
/*  Copyright (C) 2009 Universitat Politecnica de Catalunya                                   */
/**********************************************************************************************/
/*
 * Copyright (c) 2007 The Unbalanced Tree Search (UTS) Project Team:
 * -----------------------------------------------------------------
 *  
 *  This file is part of the unbalanced tree search benchmark.  This
 *  project is licensed under the MIT Open Source license.  See the LICENSE
 *  file for copyright and licensing information.
 *
 *  UTS is a collaborative project between researchers at the University of
 *  Maryland, the University of North Carolina at Chapel Hill, and the Ohio
 *  State University.
 *
 * University of Maryland:
 *   Chau-Wen Tseng(1)  <tseng at cs.umd.edu>
 *
 * University of North Carolina, Chapel Hill:
 *   Jun Huan         <huan,
 *   Jinze Liu         liu,
 *   Stephen Olivier   olivier,
 *   Jan Prins*        prins at cs.umd.edu>
 * 
 * The Ohio State University:
 *   James Dinan      <dinan,
 *   Gerald Sabin      sabin,
 *   P. Sadayappan*    saday at cse.ohio-state.edu>
 *
 * Supercomputing Research Center
 *   D. Pryor
 *
 * (1) - indicates project PI
 *
 * UTS Recursive Depth-First Search (DFS) version developed by James Dinan
 *
 * Adapted for OpenMP 3.0 Task-based version by Stephen Olivier
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 */

/* Ananya Muddukrishna (ananya@kth.se) ported to MIR */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <omp.h>
#include <sys/time.h>

#include "uts.h"
#include "helper.h"
#include "mir_public_int.h"

/***********************************************************
 *  Global state                                           *
 ***********************************************************/
unsigned long long nLeaves = 0;
int maxTreeDepth = 0;
unsigned long long num_nodes = 0 ;
/***********************************************************
 * Tree generation strategy is controlled via various      *
 * parameters set from the command line.  The parameters   *
 * and their default values are given below.               *
 * Trees are generated using a Galton-Watson process, in   *
 * which the branching factor of each node is a random     *
 * variable.                                               *
 *                                                         *
 * The random variable follow a binomial distribution.     *
 ***********************************************************/
double b_0   = 4.0; // default branching factor at the root
int   rootId = 0;   // default seed for RNG state at root
/***********************************************************
 *  The branching factor at the root is specified by b_0.
 *  The branching factor below the root follows an 
 *     identical binomial distribution at all nodes.
 *  A node has m children with prob q, or no children with 
 *     prob (1-q).  The expected branching factor is q * m.
 *
 *  Default parameter values 
 ***********************************************************/
int    nonLeafBF   = 4;            // m
double nonLeafProb = 15.0 / 64.0;  // q
/***********************************************************
 * compute granularity - number of rng evaluations per
 * tree node
 ***********************************************************/
int computeGranularity = 1;
/***********************************************************
 * expected results for execution
 ***********************************************************/
unsigned long long  exp_tree_size = 0;
int        exp_tree_depth = 0;
unsigned long long  exp_num_leaves = 0;
/***********************************************************
 *  FUNCTIONS                                              *
 ***********************************************************/

// Interpret 32 bit positive integer as value on [0,1)
double rng_toProb(int n)
{
    if (n < 0) {
        printf("*** toProb: rand n = %d out of range\n",n);
    }
    return ((n<0)? 0.0 : ((double) n)/2147483648.0);
}

void uts_initRoot(Node * root)
{
    root->height = 0;
    root->numChildren = -1;      // means not yet determined
    rng_init(root->state.state, rootId);

    PMSG("Root node at %p\n", root);
}

int uts_numChildren_bin(Node * parent)
{
    // distribution is identical everywhere below root
    int    v = rng_rand(parent->state.state);	
    double d = rng_toProb(v);

    return (d < nonLeafProb) ? nonLeafBF : 0;
}

int uts_numChildren(Node *parent)
{
    int numChildren = 0;

    /* Determine the number of children */
    if (parent->height == 0) numChildren = (int) floor(b_0);
    else numChildren = uts_numChildren_bin(parent);

    // limit number of children
    // only a BIN root can have more than MAXNUMCHILDREN
    if (parent->height == 0) {
        int rootBF = (int) ceil(b_0);
        if (numChildren > rootBF) {
            PDBG("*** Number of children of root truncated from %d to %d\n", numChildren, rootBF);
            numChildren = rootBF;
        }
    }
    else {
        if (numChildren > MAXNUMCHILDREN) {
            PDBG("*** Number of children truncated from %d to %d\n", numChildren, MAXNUMCHILDREN);
            numChildren = MAXNUMCHILDREN;
        }
    }

    return numChildren;
}

/***********************************************************
 * Recursive depth-first implementation                    *
 ***********************************************************/

unsigned long long parallel_uts ( Node *root )
{
    root->numChildren = uts_numChildren(root);

    PMSG("Computing Unbalance Tree Search algorithm ");

    num_nodes = parTreeSearch( 0, root, root->numChildren );

    PMSG(" completed!");

    return num_nodes;
}

struct  nanos_args_2_t
{/*{{{*/
  int mcc_vla_1;
  void *partialCount;
  int depth;
  Node *nodePtr;
  int i;
};/*}}}*/

/*static*/ void smp_ol_parTreeSearch_2_unpacked(const int *const mcc_vla_1, unsigned long long int *const partialCount, int *const depth, Node **const nodePtr, int *const i)
{/*{{{*/
  {
    partialCount[(*i)] = parTreeSearch((*depth) + 1, (*nodePtr), (*(*nodePtr)).numChildren);
  }
}/*}}}*/

/*static*/ void smp_ol_parTreeSearch_2(struct nanos_args_2_t *const args)
{/*{{{*/
  {
    smp_ol_parTreeSearch_2_unpacked(&((*args).mcc_vla_1), *((unsigned long long int (*)[(*args).mcc_vla_1])(*args).partialCount), &((*args).depth), &((*args).nodePtr), &((*args).i));
  }
}/*}}}*/

unsigned long long parTreeSearch(int depth, Node *parent, int numChildren) 
{
    Node n[numChildren], *nodePtr;
    int i, j;
    unsigned long long subtreesize = 1, partialCount[numChildren];

    // Recurse on the children
    for (i = 0; i < numChildren; i++) {
        nodePtr = &n[i];

        nodePtr->height = parent->height + 1;

        // The following line is the work (one or more SHA-1 ops)
        for (j = 0; j < computeGranularity; j++) {
            rng_spawn(parent->state.state, nodePtr->state.state, i);
        }

        nodePtr->numChildren = uts_numChildren(nodePtr);

        struct nanos_args_2_t imm_args;
        imm_args.mcc_vla_1 = numChildren;
        imm_args.partialCount = &partialCount;
        imm_args.depth = depth;
        imm_args.nodePtr = nodePtr;
        imm_args.i = i;

        mir_task_create((mir_tfunc_t) smp_ol_parTreeSearch_2, (void*) &imm_args, sizeof(struct nanos_args_2_t), 0, NULL, "smp_ol_parTreeSearch_2");
    }

    mir_task_wait();

    for (i = 0; i < numChildren; i++) {
        subtreesize += partialCount[i];
    }

    return subtreesize;
}

void uts_read_file ( char *filename )
{
    FILE *fin;

    if ((fin = fopen(filename, "r")) == NULL) {
        PMSG("Could not open input file (%s)\n", filename);
        exit (-1);
    }
    fscanf(fin,"%lf %lf %d %d %d %llu %d %llu",
            &b_0,
            &nonLeafProb,
            &nonLeafBF,
            &rootId,
            &computeGranularity,
            &exp_tree_size,
            &exp_tree_depth,
            &exp_num_leaves
          );
    fclose(fin);

    computeGranularity = max(1,computeGranularity);

    // Printing input data
    PMSG("\n");
    PMSG("Root branching factor                = %f\n", b_0);
    PMSG("Root seed (0 <= 2^31)                = %d\n", rootId);
    PMSG("Probability of non-leaf node         = %f\n", nonLeafProb);
    PMSG("Number of children for non-leaf node = %d\n", nonLeafBF);
    PMSG("E(n)                                 = %f\n", (double) ( nonLeafProb * nonLeafBF ) );
    PMSG("E(s)                                 = %f\n", (double) ( 1.0 / (1.0 - nonLeafProb * nonLeafBF) ) );
    PMSG("Compute granularity                  = %d\n", computeGranularity);
    PMSG("Random number generator              = "); rng_showtype();
}

void uts_show_stats()
{
    PMSG("\n");
    PMSG("Tree size                            = %llu\n", (unsigned long long)  num_nodes );
    PMSG("Maximum tree depth                   = %d\n", maxTreeDepth );
    PMSG("Chunk size                           = %d\n", 0);
    PMSG("Number of leaves                     = %llu (%.2f%%)\n", nLeaves, nLeaves/(float)num_nodes*100.0 ); 
}

int uts_check_result ( void )
{
    int answer = TEST_SUCCESSFUL;

    if ( num_nodes != exp_tree_size ) {
        answer = TEST_UNSUCCESSFUL;
        PMSG("Incorrect tree size result (%llu instead of %llu).\n", num_nodes, exp_tree_size);
    }

    return answer;
}
