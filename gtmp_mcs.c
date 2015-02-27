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

    shared nodes : array [0..P-1] of treenode
        // nodes[vpid] is allocated in shared memory
        // locally accessible to processor vpid
    processor private vpid : integer // a unique virtual processor index
    processor private sense : Boolean

    // on processor i, sense is initially true
    // in nodes[i]:
    //    havechild[j] = true if 4 * i + j + 1 < P; otherwise false
    //    parentpointer = &nodes[floor((i-1)/4].childnotready[(i-1) mod 4],
    //        or dummy if i = 0
    //    childpointers[0] = &nodes[2*i+1].parentsense, or &dummy if 2*i+1 >= P
    //    childpointers[1] = &nodes[2*i+2].parentsense, or &dummy if 2*i+2 >= P
    //    initially childnotready = havechild and parentsense = false

    procedure tree_barrier
        with nodes[vpid] do
	    repeat until childnotready = {false, false, false, false}
	    childnotready := havechild //prepare for next barrier
	    parentpointer^ := false //let parent know I'm ready
	    // if not root, wait until my parent signals wakeup
	    if vpid != 0
	        repeat until parentsense = sense
	    // signal children in wakeup tree
	    childpointers[0]^ := sense
	    childpointers[1]^ := sense
	    sense := not sense
*/


typedef struct mcs_node{
	int parentsense;
	int *parentpointer;
	int *childpointers[2];

	int havechild[4];
	int childnotready[4];
	int sense;
} mcs_node;
typedef mcs_node *mcs_node_t;

static mcs_node_t *nodes;
static int dummy;
void gtmp_init(int num_threads){
	nodes = (mcs_node_t *) malloc(sizeof(mcs_node_t) * num_threads);
	int i, j;
	for(i = 0; i < num_threads; i++) {
		nodes[i] = (mcs_node_t) malloc(sizeof(mcs_node));
		nodes[i]->parentsense = 0;
		nodes[i]->sense = 1;
		for(j = 0; j < 4; j++) {
			nodes[i]->havechild[j] = (4 * i + j + 1 < num_threads);
			nodes[i]->childnotready[j] = nodes[i]->havechild[j];
		}
	}

}

void gtmp_barrier(){

}

void gtmp_finalize(){

}



// mcs_node_t create_node() {
// 	mcs_node_t node = (mcs_node_t) malloc(sizeof(mcs_node));
// 	node->parent_sense = 1;
// 	node->parent_pinter = 0;
//
// }
