#include <iostream>
#include <omp.h>
#include <unistd.h>

#include "mir_public_int.h"
#include "helper.h"

using namespace std;

class POBJ
{
    public:
    POBJ()
    {
        POBJ(42);
    }

    ~POBJ() {}

    private:
    POBJ(int num_tasks)
    {
#pragma omp parallel
        {
#pragma omp single
            {
                for(int i=0; i<num_tasks; i++)
                {
#pragma omp task firstprivate(i)
                    cout << "T" << i << endl;
                }
#pragma omp taskwait
            }
        }
    }

};

int main(int argc, char* argv[])
{
    POBJ p;

    return 0;
}
