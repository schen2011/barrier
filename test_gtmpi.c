#include "mpi.h"
#include <stdio.h>
#include <stdlib.h>
#define  MASTER		0

static int g;

int main (int argc, char *argv[])
{
    int  numtasks, taskid, len;
    char hostname[MPI_MAX_PROCESSOR_NAME];
    int  partner, message;
    MPI_Status status;
    int n;
    n = 0;
    g = 0;

    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &numtasks);
    MPI_Comm_rank(MPI_COMM_WORLD,&taskid);
    MPI_Get_processor_name(hostname, &len);
    printf ("Hello from task %d on %s!\n", taskid, hostname);
    if (taskid == MASTER)
       printf("MASTER: Number of MPI tasks is: %d\n",numtasks);

    /* determine partner and then send/receive with partner */
    if (taskid < numtasks/2) {
      partner = numtasks/2 + taskid;
      MPI_Send(&taskid, 1, MPI_INT, partner, 1, MPI_COMM_WORLD);
      MPI_Recv(&message, 1, MPI_INT, partner, 1, MPI_COMM_WORLD, &status);
      n = n + message;
      printf("Task %d received message from task %d\n",taskid,message);

    }
    else if (taskid >= numtasks/2) {
      partner = taskid - numtasks/2;
      MPI_Recv(&message, 1, MPI_INT, partner, 1, MPI_COMM_WORLD, &status);
      MPI_Send(&taskid, 1, MPI_INT, partner, 1, MPI_COMM_WORLD);
    }
    g = g + 10 * message + 1;

    /* print partner info and exit*/
    printf("Task %d is partner with %d, n = %d, g = %d\n",taskid,message, n, g);

    MPI_Finalize();

}
