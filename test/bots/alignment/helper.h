#ifndef HELPER_H_ATGBOPN5
#define HELPER_H_ATGBOPN5

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

#ifdef DEBUG_ME
#define PDBG(...) do{ fprintf( stderr, __VA_ARGS__ ); } while(0)
#else
#define PDBG(...) do{ } while (0)
#endif

#define PMSG(...) do{ fprintf( stderr, __VA_ARGS__ ); } while(0)

#define PABRT(...) do{ fprintf( stderr, __VA_ARGS__ ); exit(1); } while(0)

#define TEST_NOT_PERFORMED 2
#define TEST_NOT_APPLICABLE 2
#define TEST_UNSUCCESSFUL 1
#define TEST_SUCCESSFUL 0

#define FALSE 0
#define TRUE 1

#endif /* end of include guard: HELPER_H_ATGBOPN5 */

