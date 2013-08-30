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

/* Ananya Muddukrishna (ananya@kth.se) ported to MIR */

#ifndef SPARSELU_H
#define SPARSELU_H

#define EPSILON 1.0E-6

int checkmat (float *M, float *N);
void genmat (float *M[]);
void print_structure(char *name, float *M[]);
float * allocate_clean_block();
void lu0(float *diag);
void bdiv(float *diag, float *row);
void bmod(float *row, float *col, float *inner);
void fwd(float *diag, float *col);

void sparselu_init(float ***pM, char *pass);
void sparselu_fini(float **M, char *pass);
void sparselu_seq_call(float **SEQ);
void sparselu_par_call(float **BENCH);
int sparselu_check(float **SEQ, float **BENCH);

#endif
