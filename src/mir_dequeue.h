/* -----------------------------------------------------------------------------
 *
 * (c) The GHC Team, 2009
 *
 * Work-stealing Deque data structure
 * 
 * ---------------------------------------------------------------------------*/
/*
The Glasgow Haskell Compiler License

Copyright 2002, The University Court of the University of Glasgow. 
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

- Redistributions of source code must retain the above copyright notice,
this list of conditions and the following disclaimer.
 
- Redistributions in binary form must reproduce the above copyright notice,
this list of conditions and the following disclaimer in the documentation
and/or other materials provided with the distribution.
 
- Neither name of the University nor the names of its contributors may be
used to endorse or promote products derived from this software without
specific prior written permission. 

THIS SOFTWARE IS PROVIDED BY THE UNIVERSITY COURT OF THE UNIVERSITY OF
GLASGOW AND THE CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES,
INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND
FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
UNIVERSITY COURT OF THE UNIVERSITY OF GLASGOW OR THE CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
DAMAGE.
*/
/* Refactored by Ananya Muddukrishna (ananya@kth.se) for MIR runtime system */

#ifndef MIR_DEQUEUE_H
#define MIR_DEQUEUE_H

#include <stdlib.h>
#include "mir_utils.h"
#include "mir_memory.h"
#include "mir_types.h"

BEGIN_C_DECLS 

#define EXTERN_INLINE static inline 
#define barf MIR_ABORT
#define ASSERT MIR_ASSERT
#define stgMallocBytes(s,m) mir_malloc_int((s))
#define stgFree(p) mir_free_int((p),0)
typedef int StgInt;
typedef unsigned int StgWord;
typedef StgWord* StgPtr;
typedef StgWord volatile*  StgVolatilePtr;   /* pointer to volatile word   */
typedef unsigned int nat;           /* at least 32 bits (like int) */
typedef enum { 
    rtsFalse = 0, 
    rtsTrue 
} rtsBool;
typedef void* StgClosurePtr;

EXTERN_INLINE StgWord
cas(StgVolatilePtr p, StgWord o, StgWord n)
{/*{{{*/
#if i386_HOST_ARCH || x86_64_HOST_ARCH
    __asm__ __volatile__ (
 	  "lock\ncmpxchg %3,%1"
          :"=a"(o), "+m" (*(volatile unsigned int *)p)
          :"0" (o), "r" (n));
    return o;
#elif powerpc_HOST_ARCH
    StgWord result;
    __asm__ __volatile__ (
        "1:     lwarx     %0, 0, %3\n"
        "       cmpw      %0, %1\n"
        "       bne       2f\n"
        "       stwcx.    %2, 0, %3\n"
        "       bne-      1b\n"
        "2:"
        :"=&r" (result)
        :"r" (o), "r" (n), "r" (p)
        :"cc", "memory"
    );
    return result;
#elif sparc_HOST_ARCH
    __asm__ __volatile__ (
	"cas [%1], %2, %0"
	: "+r" (n)
	: "r" (p), "r" (o)
	: "memory"
    );
    return n;
#elif arm_HOST_ARCH && defined(arm_HOST_ARCH_PRE_ARMv6)
    StgWord r;
    arm_atomic_spin_lock();
    r  = *p; 
    if (r == o) { *p = n; } 
    arm_atomic_spin_unlock();
    return r;
#elif arm_HOST_ARCH && !defined(arm_HOST_ARCH_PRE_ARMv6)
    StgWord result,tmp;

    __asm__ __volatile__(
        "1:     ldrex   %1, [%2]\n"
        "       mov     %0, #0\n"
        "       teq     %1, %3\n"
        "       it      eq\n"
        "       strexeq %0, %4, [%2]\n"
        "       teq     %0, #1\n"
        "       it      eq\n"
        "       beq     1b\n"
#if !defined(arm_HOST_ARCH_PRE_ARMv7)
        "       dmb\n"
#endif
                : "=&r"(tmp), "=&r"(result)
                : "r"(p), "r"(o), "r"(n)
                : "cc","memory");

    return result;
#elif !defined(WITHSMP)
    StgWord result;
    result = *p;
    if (result == o) {
        *p = n;
    }
    return result;
#else
#error cas() unimplemented on this architecture
#endif
}/*}}}*/

EXTERN_INLINE StgWord
atomic_inc(StgVolatilePtr p, StgWord incr)
{/*{{{*/
#if defined(i386_HOST_ARCH) || defined(x86_64_HOST_ARCH)
    StgWord r;
    r = incr;
    __asm__ __volatile__ (
        "lock\nxadd %0,%1":
            "+r" (r), "+m" (*p):
    );
    return r + incr;
#else
    StgWord old, new;
    do {
        old = *p;
        new = old + incr;
    } while (cas(p, old, new) != old);
    return new;
#endif
}/*}}}*/

EXTERN_INLINE StgWord
atomic_dec(StgVolatilePtr p)
{/*{{{*/
#if defined(i386_HOST_ARCH) || defined(x86_64_HOST_ARCH)
    StgWord r;
    r = (StgWord)-1;
    __asm__ __volatile__ (
        "lock\nxadd %0,%1":
            "+r" (r), "+m" (*p):
    );
    return r-1;
#else
    StgWord old, new;
    do {
        old = *p;
        new = old - 1;
    } while (cas(p, old, new) != old);
    return new;
#endif
}/*}}}*/

EXTERN_INLINE void
busy_wait_nop(void)
{/*{{{*/
#if defined(i386_HOST_ARCH) || defined(x86_64_HOST_ARCH)
    __asm__ __volatile__ ("rep; nop");
    //
#else
    // nothing
#endif
}/*}}}*/

/*
 * We need to tell both the compiler AND the CPU about the barriers.
 * It's no good preventing the CPU from reordering the operations if
 * the compiler has already done so - hence the "memory" restriction
 * on each of the barriers below.
 */
EXTERN_INLINE void
write_barrier(void) 
{/*{{{*/
#if i386_HOST_ARCH || x86_64_HOST_ARCH
    __asm__ __volatile__ ("" : : : "memory");
#elif powerpc_HOST_ARCH
    __asm__ __volatile__ ("lwsync" : : : "memory");
#elif sparc_HOST_ARCH
    /* Sparc in TSO mode does not require store/store barriers. */
    __asm__ __volatile__ ("" : : : "memory");
#elif arm_HOST_ARCH && defined(arm_HOST_ARCH_PRE_ARMv7)
    __asm__ __volatile__ ("" : : : "memory");
#elif arm_HOST_ARCH && !defined(arm_HOST_ARCH_PRE_ARMv7)
    __asm__ __volatile__ ("dmb  st" : : : "memory");
#elif !defined(WITHSMP)
    return;
#else
#error memory barriers unimplemented on this architecture
#endif
}/*}}}*/

EXTERN_INLINE void
store_load_barrier(void) 
{/*{{{*/
#if i386_HOST_ARCH
    __asm__ __volatile__ ("lock; addl $0,0(%%esp)" : : : "memory");
#elif x86_64_HOST_ARCH
    __asm__ __volatile__ ("lock; addq $0,0(%%rsp)" : : : "memory");
#elif powerpc_HOST_ARCH
    __asm__ __volatile__ ("sync" : : : "memory");
#elif sparc_HOST_ARCH
    __asm__ __volatile__ ("membar #StoreLoad" : : : "memory");
#elif arm_HOST_ARCH && !defined(arm_HOST_ARCH_PRE_ARMv7)
    __asm__ __volatile__ ("dmb" : : : "memory");
#elif !defined(WITHSMP)
    return;
#else
#error memory barriers unimplemented on this architecture
#endif
}/*}}}*/

EXTERN_INLINE void
load_load_barrier(void) 
{/*{{{*/
#if i386_HOST_ARCH
    __asm__ __volatile__ ("" : : : "memory");
#elif x86_64_HOST_ARCH
    __asm__ __volatile__ ("" : : : "memory");
#elif powerpc_HOST_ARCH
    __asm__ __volatile__ ("lwsync" : : : "memory");
#elif sparc_HOST_ARCH
    /* Sparc in TSO mode does not require load/load barriers. */
    __asm__ __volatile__ ("" : : : "memory");
#elif arm_HOST_ARCH && !defined(arm_HOST_ARCH_PRE_ARMv7)
    __asm__ __volatile__ ("dmb" : : : "memory");
#elif !defined(WITHSMP)
    return;
#else
#error memory barriers unimplemented on this architecture
#endif
}/*}}}*/

// Load a pointer from a memory location that might be being modified
// concurrently.  This prevents the compiler from optimising away
// multiple loads of the memory location, as it might otherwise do in
// a busy wait loop for example.
#define VOLATILE_LOAD(p) (*((StgVolatilePtr)(p)))

typedef struct WSDeque_ {
    // Size of elements array. Used for modulo calculation: we round up
    // to powers of 2 and use the dyadic log (modulo == bitwise &) 
    StgWord size; 
    StgWord moduloSize; /* bitmask for modulo */

    // top, index where multiple readers steal() (protected by a cas)
    volatile StgWord top;

    // bottom, index of next free place where one writer can push
    // elements. This happens unsynchronised.
    volatile StgWord bottom;

    // both top and bottom are continuously incremented, and used as
    // an index modulo the current array size.
  
    // lower bound on the current top value. This is an internal
    // optimisation to avoid unnecessarily accessing the top field
    // inside pushBottom
    volatile StgWord topBound;

    // The elements array
    void ** elements;

    //  Please note: the dataspace cannot follow the admin fields
    //  immediately, as it should be possible to enlarge it without
    //  disposing the old one automatically (as realloc would)!

} WSDeque;
typedef WSDeque mir_dequeue_t;

/* INVARIANTS, in this order: reasonable size,
   topBound consistent, space pointer, space accessible to us.
   
   NB. This is safe to use only (a) on a spark pool owned by the
   current thread, or (b) when there's only one thread running, or no
   stealing going on (e.g. during GC).
*/
#define ASSERT_WSDEQUE_INVARIANTS(p)         \
  ASSERT((p)->size > 0);                        \
  ASSERT((p)->topBound <= (p)->top);            \
  ASSERT((p)->elements != NULL);                \
  ASSERT(*((p)->elements) || 1);                \
  ASSERT(*((p)->elements - 1  + ((p)->size)) || 1);

// No: it is possible that top > bottom when using pop()
//  MIR_ASSERT((p)->bottom >= (p)->top);           
//  MIR_ASSERT((p)->size > (p)->bottom - (p)->top);

/* -----------------------------------------------------------------------------
 * Operations
 *
 * A WSDeque has an *owner* thread.  The owner can perform any operation;
 * other threads are only allowed to call stealWSDeque_(),
 * stealWSDeque(), looksEmptyWSDeque(), and dequeElements().
 *
 * -------------------------------------------------------------------------- */

// Allocation, deallocation
WSDeque * newWSDeque  (nat size);
void      freeWSDeque (WSDeque *q);

// Take an element from the "write" end of the pool.  Can be called
// by the pool owner only.
void* popWSDeque (WSDeque *q);

// Push onto the "write" end of the pool.  Return true if the push
// succeeded, or false if the deque is full.
rtsBool pushWSDeque (WSDeque *q, void *elem);

// Removes all elements from the deque
EXTERN_INLINE void discardElements (WSDeque *q);

// Removes an element of the deque from the "read" end, or returns
// NULL if the pool is empty, or if there was a collision with another
// thief.
void * stealWSDeque_ (WSDeque *q);

// Removes an element of the deque from the "read" end, or returns
// NULL if the pool is empty.
void * stealWSDeque (WSDeque *q);

// "guesses" whether a deque is empty. Can return false negatives in
//  presence of concurrent steal() calls, and false positives in
//  presence of a concurrent pushBottom().
EXTERN_INLINE rtsBool looksEmptyWSDeque (WSDeque* q);

EXTERN_INLINE long dequeElements   (WSDeque *q);

/* -----------------------------------------------------------------------------
 * PRIVATE below here
 * -------------------------------------------------------------------------- */

EXTERN_INLINE long
dequeElements (WSDeque *q)
{/*{{{*/
    StgWord t = q->top;
    StgWord b = q->bottom;
    // try to prefer false negatives by reading top first
    return ((long)b - (long)t);
}/*}}}*/

EXTERN_INLINE rtsBool
looksEmptyWSDeque (WSDeque *q)
{/*{{{*/
    return (dequeElements(q) <= 0);
}/*}}}*/

EXTERN_INLINE void
discardElements (WSDeque *q)
{/*{{{*/
    q->top = q->bottom;
//    pool->topBound = pool->top;
}/*}}}*/

END_C_DECLS 
#endif // MIR_DEQUEUE_H 
