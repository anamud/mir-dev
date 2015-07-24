#ifndef MIR_UTILS_H
#define MIR_UTILS_H 1

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <assert.h>
#include "mir_types.h"

#define MIR_ASSERT(cond) assert(cond)

// Inspired from Zeds awesome debug macros.
// http://c.learncodethehardway.org/book/ex20.html

#include <errno.h>
#include <string.h>

#ifndef MIR_ENABLE_DEBUG
#define MIR_DEBUG(M, ...)
#else
#define MIR_DEBUG(M, ...) do { fprintf(stderr, "MIR_DEBUG %s:%d: " M "\n", __FILE__, __LINE__, ##__VA_ARGS__); } while(0)
#endif

#define clean_errno() (errno == 0 ? "None" : strerror(errno))

#define MIR_LOG_ERR(M, ...) do { fprintf(stderr, "[MIR_ERROR] (%s:%d: errno: %s) " M "\n", __FILE__, __LINE__, clean_errno(), ##__VA_ARGS__); exit(1); } while(0)

#define MIR_LOG_WARN(M, ...) do { fprintf(stderr, "[MIR_WARNING] (%s:%d: errno: %s) " M "\n", __FILE__, __LINE__, clean_errno(), ##__VA_ARGS__); } while(0)

#define MIR_LOG_INFO(M, ...) fprintf(stderr, "[MIR_INFO] (%s:%d) " M "\n", __FILE__, __LINE__, ##__VA_ARGS__)

#define MIR_ASSERT_STR(A, M, ...) if(!(A)) { MIR_LOG_ERR(M, ##__VA_ARGS__); MIR_ASSERT(A); }

#define MIR_CHECK_MEM(A) MIR_ASSERT_STR((A), "Out of memory.")

#define MIR_CHECK_FILE(A) MIR_ASSERT_STR((A), "File open error.")

BEGIN_C_DECLS

int mir_pstack_set_size(size_t sz);

/*LIBINT*/ int mir_get_num_threads();

/*LIBINT*/ int mir_get_threadid();

/*LIBINT*/ void mir_sleep_ms(uint32_t msec);

/*LIBINT*/ void mir_sleep_us(uint32_t usec);

/*LIBINT*/ uint64_t mir_get_cycles();

END_C_DECLS

#endif
