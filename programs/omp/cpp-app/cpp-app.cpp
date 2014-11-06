#include <iostream>
#include <omp.h>
#include <unistd.h>

#define USE_MIR 

#ifdef USE_MIR
#include "mir_public_int.h"
#include "helper.h"
#endif

using namespace std;

class POBJ
{
    public:
    POBJ()
    {
        POBJ(sysconf(_SC_NPROCESSORS_ONLN));
    }

    ~POBJ() {}

    private:
    POBJ(int num_threads)
    {
//#pragma omp parallel num_threads(num_threads)
        //{
            //cout << "P-" << mir_get_threadid() << endl;
//#pragma omp single
            //{
                //cout << "S-" << mir_get_threadid() << endl;
                for(int i=0; i<4; i++)
                {
#pragma omp task firstprivate(i)
                    cout << "T" << i << endl;
                }
#pragma omp taskwait
            //}
        //}
    }

};

int main(int argc, char* argv[])
{
#ifdef USE_MIR
    mir_create();
#endif

    POBJ p;

#ifdef USE_MIR
    mir_destroy();
#endif
    return 0;
}
