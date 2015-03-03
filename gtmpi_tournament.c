#include <stdlib.h>
#include <stdio.h>
#include <mpi.h>
#include "gtmpi.h"

/*
    From the MCS Paper: A scalable, distributed tournament barrier with only local spinning

    type round_t = record
        role : (winner, loser, bye, champion, dropout)
	opponent : ^Boolean
	flag : Boolean
    shared rounds : array [0..P-1][0..LogP] of round_t
        // row vpid of rounds is allocated in shared memory
	// locally accessible to processor vpid

    processor private sense : Boolean := true
    processor private vpid : integer // a unique virtual processor index

    //initially
    //    rounds[i][k].flag = false for all i,k
    //rounds[i][k].role =
    //    winner if k > 0, i mod 2^k = 0, i + 2^(k-1) < P , and 2^k < P
    //    bye if k > 0, i mode 2^k = 0, and i + 2^(k-1) >= P
    //    loser if k > 0 and i mode 2^k = 2^(k-1)
    //    champion if k > 0, i = 0, and 2^k >= P
    //    dropout if k = 0
    //    unused otherwise; value immaterial
    //rounds[i][k].opponent points to
    //    round[i-2^(k-1)][k].flag if rounds[i][k].role = loser
    //    round[i+2^(k-1)][k].flag if rounds[i][k].role = winner or champion
    //    unused otherwise; value immaterial
    procedure tournament_barrier
        round : integer := 1
	loop   //arrival
	    case rounds[vpid][round].role of
	        loser:
	            rounds[vpid][round].opponent^ :=  sense
		    repeat until rounds[vpid][round].flag = sense
		    exit loop
   	        winner:
	            repeat until rounds[vpid][round].flag = sense
		bye:  //do nothing
		champion:
	            repeat until rounds[vpid][round].flag = sense
		    rounds[vpid][round].opponent^ := sense
		    exit loop
		dropout: // impossible
	    round := round + 1
	loop  // wakeup
	    round := round - 1
	    case rounds[vpid][round].role of
	        loser: // impossible
		winner:
		    rounds[vpid[round].opponent^ := sense
		bye: // do nothing
		champion: // impossible
		dropout:
		    exit loop
	sense := not sense
*/
typedef enum {winner, loser, bye, champion, dropout, unused} role_t;
typedef struct _round_t{
    role_t role;
    int* opponent;
    int flag;
} round_t;

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

static round_t** rounds;
static int* sense_array;

void gtmpi_init(int num_threads){
    int i, k, P, LogP;
    role_t role;
    P = num_threads;
    LogP = log2(P);
    rounds = (round_t**) malloc(num_threads * sizeof(round_t*));
    for (i = 0; i < P; i++) {
        rounds[i] = (round_t*) malloc(LogP * sizeof(round_t));
        for (k = 0; k < LogP; k++) {
            rounds[i][k].flag = 0;
            role = unused;
            if (role == unused && k > 0 && i % power(2,k) == 0 && i + power(2, k-1) < P && power(2,k) < P)
                role = winner;
            if (role == unused && k > 0 && i % power(2,k) == 0 && i + power(2, k-1) >= P)
                role = bye;
            if (role == unused && k > 0 && i % power(2,k) == power(2, k-1))
                role = loser;
            if (role == unused && k > 0 && i == 0 && power(2,k) >= P)
                role = champion;
            if (role == unused && k == 0)
                role = dropout;
            rounds[i][k].role = role;
        }
    }

    for (i = 0; i < P; i++) {
        for (k = 0; k < LogP; k++) {
            switch(rounds[i][k].role) {
                case loser:
                    rounds[i][k].opponent = &(rounds[i-power(2,k-1)][k].flag);
                    break;
                case winner:
                case champion:
                    rounds[i][k].opponent = &(rounds[i+power(2,k-1)][k].flag);
                    break;
                default:
                    rounds[i][k].opponent = NULL;
            }
        }
    }

    sense_array = (int *) malloc(P * sizeof(int));
    for (i = 0; i < P; i++)
        sense_array[i] = 1;
}

void gtmpi_barrier(){
    int round, loop, sense;
    role_t role;
    int vpid,num_processes;
    MPI_Comm_size(MPI_COMM_WORLD,&num_processes);
    MPI_Comm_rank(MPI_COMM_WORLD,&vpid);
    round = 1;
    loop = 1;
    sense = sense_array[vpid];
    while(loop) {
        role = rounds[vpid][round];
        switch(role) {
            loser:
                *(rounds[vpid][round].opponent) = sense;
                while (rounds[vpid][round].flag == sense);
                loop = 0;
                break;
            winner:
                while (rounds[vpid][round].flag == sense);
                break;
            bye:
                break;
            champion:
                while (rounds[vpid][round].flag == sense);
                *(rounds[vpid][round].opponent) = sense;
                loop = 0;
                break;
            dropout:
                printf("dropout in loop arrival, impossible\n"); fflush(stdout);
        }
        round++;
    }
    loop = 1;
    while(loop) {
        round--;
        role = rounds[vpid][round];
        switch(role) {
            loser:
                printf("loser in loop departure, impossible\n"); fflush(stdout);
                loop = 0;
                break;
            winner:
                *(rounds[vpid][round].opponent) = sense;
                break;
            bye:
                break;
            champion:
                printf("champion in loop departure, impossible\n"); fflush(stdout);
                loop = 0;
                break;
            dropout:
                loop = 0;
                break;
        }
    }
    sense_array[vpid] = !sense;
}

void gtmpi_finalize(){
    free(rounds);

}
