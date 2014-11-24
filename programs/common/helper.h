#ifndef HELPER_H_ATGBOPN5
#define HELPER_H_ATGBOPN5

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

#ifdef ALLOW_DEBUG_MESSAGES
#define PDBG(...) do{ fprintf( stderr, __VA_ARGS__ ); } while(0)
#else
#define PDBG(...) do{ } while (0)
#endif

#ifdef ALLOW_MESSAGES
#define PMSG(...) do{ fprintf( stderr, __VA_ARGS__ ); } while(0)
#else
#define PMSG(...) do{ } while (0)
#endif

#define PALWAYS(...) do{ fprintf( stderr, __VA_ARGS__ ); } while(0)

#define PABRT(...) do{ fprintf( stderr, __VA_ARGS__ ); exit(1); } while(0)

#define TEST_NOT_PERFORMED 2
#define TEST_UNSUCCESSFUL 1
#define TEST_SUCCESSFUL 0
#define TEST_ENUM_STRING "[SUCCESSFUL=0, UNSUCCESSFUL, NOT_PERFORMED]"

#ifndef FALSE
#define FALSE 0
#endif
#ifndef TRUE
#define TRUE 1
#endif

//// This is useful for pin-tool profiling
//#ifdef NOINLINE_TASK
////#if defined __INTEL_COMPILER
////#define ATTR_NOINLINE
////#elif defined __GUNC__
//#define ATTR_NOINLINE __attribute__((noinline))
////#endif
//#else
//#define ATTR_NOINLINE
//#endif

#endif /* end of include guard: HELPER_H_ATGBOPN5 */

