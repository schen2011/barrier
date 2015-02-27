#include <omp.h>
#include "gtmp.h"

/*
    From the MCS Paper: A sense-reversing centralized barrier

    shared count : integer := P
    shared sense : Boolean := true
    processor private local_sense : Boolean := true

    procedure central_barrier
        local_sense := not local_sense // each processor toggles its own sense
	if fetch_and_decrement (&count) = 1
	    count := P
	    sense := local_sense // last processor toggles global sense
        else
           repeat until sense = local_sense
*/

static int count;
static int global_sense;
static int *local_sense;


void gtmp_init(int num_threads){
    count = num_threads;
    global_sense = 1;
    local_sense = malloc(num_threads * sizeof(int));
    int i;
    for(i= 0; i<num_threads; i++)
        local_sense[i] = 1;
    omp_set_num_threads(num_threads);
}

void gtmp_barrier(){
    // each processor toggles its own sense
    int thread_num = omp_get_thread_num();
    local_sense[thread_num] = !local_sense[thread_num];

    int curr_local_sense = local_sense[thread_num];

    // last processor toggles global sense
    if(__sync_fetch_and_sub(&count, 1) == 1)
    {
        #pragma omp critical
        {
            count = omp_get_num_threads();
            globalsense = curr_local_sense;
        }
    }
    else {
        while(curr_local_sense != globalsense);
    }

}

void gtmp_finalize(){
    if(local_sense != NULL) {
        free(local_sense);
    }

}
