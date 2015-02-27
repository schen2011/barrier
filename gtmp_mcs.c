#include <stdlib.h>
#include <stdio.h>
#include <omp.h>
#include "gtmp.h"

/*
    From the MCS Paper: A scalable, distributed tree-based barrier with only local spinning.

    type treenode = record
        parentsense : Boolean
	parentpointer : ^Boolean
	childpointers : array [0..1] of ^Boolean
	havechild : array [0..3] of Boolean
	childnotready : array [0..3] of Boolean
	dummy : Boolean //pseudo-data

    shared record : array [0..P-1] of treenode
        // record[i] is allocated in shared memory
        // locally accessible to processor i
    processor private i : integer // a unique virtual processor index
    processor private sense : Boolean

    // on processor i, sense is initially true
    // in record[i]:
    //    havechild[j] = true if 4 * i + j + 1 < P; otherwise false
    //    parentpointer = &record[floor((i-1)/4].childnotready[(i-1) mod 4],
    //        or dummy if i = 0
    //    childpointers[0] = &record[2*i+1].parentsense, or &dummy if 2*i+1 >= P
    //    childpointers[1] = &record[2*i+2].parentsense, or &dummy if 2*i+2 >= P
    //    initially childnotready = havechild and parentsense = false

    procedure tree_barrier
        with record[i] do
	    repeat until childnotready = {false, false, false, false}
	    childnotready := havechild //prepare for next barrier
	    parentpointer^ := false //let parent know I'm ready
	    // if not root, wait until my parent signals wakeup
	    if i != 0
	        repeat until parentsense = sense
	    // signal children in wakeup tree
	    childpointers[0]^ := sense
	    childpointers[1]^ := sense
	    sense := not sense
*/


typedef struct _treenode{
	int havechild[4];
	int childnotready[4];
	int parentsense;
	int sense;
	int *parentpointer;
	int *childpointers[2];
} treenode;
typedef treenode *treenode_t;

static treenode_t *record; //shared record : array [0..P-1] of treenode
static int dummy; //Boolean //pseudo-data

void gtmp_init(int num_threads){
	int P = num_threads;
	// record[i] is allocated in shared memory
	// locally accessible to processor i
	record = (treenode_t *) malloc(sizeof(treenode_t) * P);
	dummy = -1;
	int i; // a unique virtual processor index
	int j;

	for(i = 0; i < num_threads; i++) {
		record[i] = (treenode_t) malloc(sizeof(treenode));
		for(j = 0; j < 4; j++) {
			record[i]->havechild[j] = (4 * i + j + 1 < P);
			//    initially childnotready = havechild
			record[i]->childnotready[j] = record[i]->havechild[j];
		}
		record[i]->parentsense = 0; //    initially parentsense = false
		record[i]->sense = 1; // on processor i, sense is initially true
	}
	//    parentpointer = &record[floor((i-1)/4].childnotready[(i-1) mod 4],
	//        or dummy if i = 0
	if (i == 0)
		record[i]->parentpointer = &dummy;
	else
		record[i]->parentpointer = &(record[(i-1)/4]->childnotready[(i-1)%4]);
	//    childpointers[0] = &record[2*i+1].parentsense, or &dummy if 2*i+1 >= P
    //    childpointers[1] = &record[2*i+2].parentsense, or &dummy if 2*i+2 >= P
	for (j = 0; j < 2; j++) {
		if ((2*i+j) >= P)
			record[i]->childpointers[j] = &dummy;
		else
			record[i]->childpointers[j] = &(record[2*i+j]->parentsense);
	}

}
/*
procedure tree_barrier
	with record[i] do
	repeat until childnotready = {false, false, false, false}
	childnotready := havechild //prepare for next barrier
	parentpointer^ := false //let parent know I'm ready
	// if not root, wait until my parent signals wakeup
	if i != 0
		repeat until parentsense = sense
	// signal children in wakeup tree
	childpointers[0]^ := sense
	childpointers[1]^ := sense
	sense := not sense
	*/
void gtmp_barrier(){
	int P, i, j;
	P = omp_get_num_threads();
	i = omp_get_thread_num();
	int *cn = record[i]->childnotready;
	// repeat until childnotready = {false, false, false, false}
	printf("thread[%d] wait for child ready\n", i); fflush(stdout);
	while(cn[0] || cn[1] || cn[2] || cn[3]);
	printf("thread[%d] all child ready\n", i); fflush(stdout);
	// 	childnotready := havechild //prepare for next barrier
	for (j = 0; j < 4; j++)
		record[i]->childnotready[j] = record[i]->havechild[j];

	if (i != 0) {
		// 	parentpointer^ := false
		printf("thread[%d] notify parent I'm ready\n", i); fflush(stdout);
		*(record[i]->parentpointer) = 0; //let parent know I'm ready
		// repeat until parentsense = sense
		printf("thread[%d] wait for signal\n", i); fflush(stdout);
		while (record[i]->parentsense != record[i]->sense);
	}
	// signal children in wakeup tree
	printf("thread[%d] wakes up child thread[%d]\n", i, 2*i); fflush(stdout);
	*(record[i]->childpointers[0]) = record[i]->sense; // childpointers[0]^ := sense
	printf("thread[%d] wakes up child thread[%d]\n", i, 2*i+1); fflush(stdout);
	*(record[i]->childpointers[1]) = record[i]->sense; // childpointers[1]^ := sense
	printf("thread[%d] reverse sense\n", i); fflush(stdout);
	record[i]->sense = !(record[i]->sense); // sense := not sense

}

void gtmp_finalize(){
	if(record != NULL) {
		int i, P;
		P = sizeof(record) / sizeof(record[0]);
		for (i = 0; i < P; i++) {
			if (record[i] != NULL)
				free(record[i]);
		}
		free(record);
	}

}
