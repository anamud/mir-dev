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

/* 
 * Original code from the Cilk project 
 *
 * Copyright (c) 2000 Massachusetts Institute of Technology
 * Copyright (c) 2000 Matteo Frigo
 */

/* Ananya Muddukrishna (ananya@kth.se) ported to MIR */

#ifndef FFT_H
#define FFT_H

/* our real numbers */
typedef float REAL;

/* Complex numbers and operations */
typedef struct {
     REAL re, im;
} COMPLEX;

#define c_re(c)  ((c).re)
#define c_im(c)  ((c).im)

void compute_w_coefficients(int n, int a, int b, COMPLEX * W);
void compute_w_coefficients_seq(int n, int a, int b, COMPLEX * W);
int factor(int n);
void unshuffle(int a, int b, COMPLEX * in, COMPLEX * out, int r, int m);
void unshuffle_seq(int a, int b, COMPLEX * in, COMPLEX * out, int r, int m);
void fft_twiddle_gen1(COMPLEX * in, COMPLEX * out, COMPLEX * W, int r, int m, int nW, int nWdnti, int nWdntm);
void fft_twiddle_gen(int i, int i1, COMPLEX * in, COMPLEX * out, COMPLEX * W, int nW, int nWdn, int r, int m);
void fft_twiddle_gen_seq(int i, int i1, COMPLEX * in, COMPLEX * out, COMPLEX * W, int nW, int nWdn, int r, int m);
void fft_base_2(COMPLEX * in, COMPLEX * out);
void fft_twiddle_2(int a, int b, COMPLEX * in, COMPLEX * out, COMPLEX * W, int nW, int nWdn, int m);
void fft_twiddle_2_seq(int a, int b, COMPLEX * in, COMPLEX * out, COMPLEX * W, int nW, int nWdn, int m);
void fft_unshuffle_2(int a, int b, COMPLEX * in, COMPLEX * out, int m);
void fft_unshuffle_2_seq(int a, int b, COMPLEX * in, COMPLEX * out, int m);
void fft_base_4(COMPLEX * in, COMPLEX * out);
void fft_twiddle_4(int a, int b, COMPLEX * in, COMPLEX * out, COMPLEX * W, int nW, int nWdn, int m);
void fft_twiddle_4_seq(int a, int b, COMPLEX * in, COMPLEX * out, COMPLEX * W, int nW, int nWdn, int m);
void fft_unshuffle_4(int a, int b, COMPLEX * in, COMPLEX * out, int m);
void fft_unshuffle_4_seq(int a, int b, COMPLEX * in, COMPLEX * out, int m);
void fft_base_8(COMPLEX * in, COMPLEX * out);
void fft_twiddle_8(int a, int b, COMPLEX * in, COMPLEX * out, COMPLEX * W, int nW, int nWdn, int m);
void fft_twiddle_8_seq(int a, int b, COMPLEX * in, COMPLEX * out, COMPLEX * W, int nW, int nWdn, int m);
void fft_unshuffle_8(int a, int b, COMPLEX * in, COMPLEX * out, int m);
void fft_unshuffle_8_seq(int a, int b, COMPLEX * in, COMPLEX * out, int m);
void fft_base_16(COMPLEX * in, COMPLEX * out);
void fft_twiddle_16(int a, int b, COMPLEX * in, COMPLEX * out, COMPLEX * W, int nW, int nWdn, int m);
void fft_twiddle_16_seq(int a, int b, COMPLEX * in, COMPLEX * out, COMPLEX * W, int nW, int nWdn, int m);
void fft_unshuffle_16(int a, int b, COMPLEX * in, COMPLEX * out, int m);
void fft_unshuffle_16_seq(int a, int b, COMPLEX * in, COMPLEX * out, int m);
void fft_base_32(COMPLEX * in, COMPLEX * out);
void fft_twiddle_32(int a, int b, COMPLEX * in, COMPLEX * out, COMPLEX * W, int nW, int nWdn, int m);
void fft_twiddle_32_seq(int a, int b, COMPLEX * in, COMPLEX * out, COMPLEX * W, int nW, int nWdn, int m);
void fft_unshuffle_32(int a, int b, COMPLEX * in, COMPLEX * out, int m);
void fft_unshuffle_32_seq(int a, int b, COMPLEX * in, COMPLEX * out, int m);
void fft_aux(int n, COMPLEX * in, COMPLEX * out, int *factors, COMPLEX * W, int nW);
void fft_aux_seq(int n, COMPLEX * in, COMPLEX * out, int *factors, COMPLEX * W, int nW);
void fft(int n, COMPLEX * in, COMPLEX * out);
void fft_seq(int n, COMPLEX * in, COMPLEX * out);
int test_correctness(int n, COMPLEX *out1, COMPLEX *out2);

#endif

