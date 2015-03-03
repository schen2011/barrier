#include <stdlib.h>
#include <stdio.h>
#include <mpi.h>
#include "gtmpi.h"

/*
    From the MCS Paper: The scalable, distributed dissemination barrier with only local spinning.

    type flags = record
        myflags : array [0..1] of array [0..LogP - 1] of Boolean
	partnerflags : array [0..1] of array [0..LogP - 1] of ^Boolean

    processor private parity : integer := 0
    processor private sense : Boolean := true
    processor private localflags : ^flags

    shared allnodes : array [0..P-1] of flags
        //allnodes[i] is allocated in shared memory
	//locally accessible to processor i

    //on processor i, localflags points to allnodes[i]
    //initially allnodes[i].myflags[r][k] is false for all i, r, k
    //if j = (i+2^k) mod P, then for r = 0 , 1:
    //    allnodes[i].partnerflags[r][k] points to allnodes[j].myflags[r][k]

    procedure dissemination_barrier
        for instance : integer :0 to LogP-1
	    localflags^.partnerflags[parity][instance]^ := sense
	    repeat until localflags^.myflags[parity][instance] = sense
	if parity = 1
	    sense := not sense
	parity := 1 - parity
*/

typedef struct _flags {
	myflags *int[2];
	partnerflags **int[2];
} flags;

static flags** allnodes;
static int* parity_array;
static int* sense_array;


int power(int x, unsigned int y) {
	if (y == 0)
		return 1;
	else if (y == 1)
		return x;
	else if (y % 2 == 0)
		return power(x, y/2) * power(x, y/2);
	else
		return x*power(x, y/2) * power(x, y/2);
}

int log2(int x) {
	int v, y;
	v = 1;
	y = 0;
	while( v < x){
		v *= 2;
		y++;
	}
	return y;
}

void gtmpi_init(int num_threads){
	int i, j, r, k, LogP;
	LogP = log2(num_threads);
	allnodes = (flags**) malloc(sizeof(flags*) * num_threads);
	for (i = 0; i < num_threads; i++) {
		allnodes[i] = (flags*) malloc(sizeof(flags));
		for (r = 0; r < 2; r++) {
			allnodes[i]->myflags[r] = (int *) (malloc(sizeof(int) * LogP));
			for (k = 0; k < LogP; k++)
				allnodes[i]->myflags[r][k] = 0;
		}
	}

	for (i = 0; i < num_threads; i++) {
		for (r = 0; r < 2; r++) {
			allnodes[i].partnerflags[r] = (int **) (malloc(sizeof(int*) * LogP));
			for (k = 0; k < LogP; k++){
				j = (i+power(2,k)) % num_threads;
				allnodes[i].partnerflags[r][k] = &(allnodes[j].myflags[r][k]);
			}
		}
	}


}

void gtmpi_barrier(){
	// procedure dissemination_barrier
	// 	for instance : integer :0 to LogP-1
	// 	localflags^.partnerflags[parity][instance]^ := sense
	// 	repeat until localflags^.myflags[parity][instance] = sense
	// if parity = 1
	// 	sense := not sense
	// parity := 1 - parity
	int i, j, LogP, parity, sense;
	int id,num_processes;
	flags* localflags;
	MPI_Comm_size(MPI_COMM_WORLD,&num_processes);
	MPI_Comm_rank(MPI_COMM_WORLD,&id);
	LogP = log2(num_processes);

	localflags = allnodes[id];
	parity = parity_array[id];
	sense = sense_array[id];
	for (i = 0; i < num_processes; i++) {
		*(localflags->partnerflags[parity][i]) = sense
		if (localflags->myflags[parity][i] == sense)
			break;
	}
	if (parity == 1)
		sense_array[id] = !sense;
	parity_array[id] = 1 - parity;

}

void gtmpi_finalize(){
	free(allnodes);
}
