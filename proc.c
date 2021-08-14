/*
  Name: Gregory G Danielson III
  BlazerId: gregdan3
  Course Section: CS 632
  Homework #: 5
  Instructions to compile the program:
    `make build`
    See Makefile for details
  Instructions to run the program:
    `make run`
    See Makefile for details
*/

#define _GNU_SOURCE
#include <assert.h>
#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

const int TIMES_TO_TEST = 1;

void mygather(void *sendbuf, int sendcount, MPI_Datatype sendtype,
              void *recvbuf, int recvcount, MPI_Datatype recvtype, int root,
              MPI_Comm communicator) {
  /* Each process sends an element to one process
   * only root (0) needs valid recv buffer!
   */
  int rank;
  MPI_Comm_rank(communicator, &rank);
  int size;
  MPI_Comm_size(communicator, &size);
  int sendtype_s, recvtype_s;
  MPI_Type_size(sendtype, &sendtype_s);
  MPI_Type_size(recvtype, &recvtype_s);

  MPI_Request sreq;
  MPI_Request *rreqs = malloc(size * sizeof(MPI_Request));
  MPI_Status send_stat;
  MPI_Status *recv_stats = malloc(size * sizeof(MPI_Status));

  // all procs send to root
  MPI_Isend(sendbuf, sendcount, sendtype, root, 0, communicator, &sreq);
  if (rank == root) {
    for (int i = 0; i < size; i++) {
      int recvoffset = i * recvcount * recvtype_s;
      MPI_Irecv(recvbuf + recvoffset, recvcount, recvtype, i, 0, communicator,
                &rreqs[i]);
      MPI_Wait(&rreqs[i], &recv_stats[i]);
    }
  }
  MPI_Wait(&sreq, &send_stat);
  free(rreqs);
  free(recv_stats);
}

void mybcast(void *buf, int count, MPI_Datatype type, int root,
             MPI_Comm communicator) {
  /* Copy one element to all processes
   * Tag is irrelevant; always 0 in this communication
   * */
  int rank;
  MPI_Comm_rank(communicator, &rank);
  int size;
  MPI_Comm_size(communicator, &size);

  MPI_Request *sreqs = malloc(size * sizeof(MPI_Request));
  MPI_Request rreq;
  MPI_Status *send_stats = malloc(size * sizeof(MPI_Status));
  MPI_Status recv_stat;

  if (size == 0) {
    free(sreqs);
    free(send_stats);
    return;
  }

  if (rank == root) { // root sends to all other procs
    for (int i = 1; i < size; i++) {
      MPI_Isend(buf, count * size, type, i, 0, communicator, &sreqs[i]);
      MPI_Wait(&sreqs[i], &send_stats[i]);
    }
  } else { // all other procs wait for root
    MPI_Irecv(buf, count * size, type, root, 0, communicator, &rreq);
    MPI_Wait(&rreq, &recv_stat);
  }
  free(sreqs);
  free(send_stats);
}

void allgather(void *sendbuf, int sendcount, MPI_Datatype sendtype,
               void *recvbuf, int recvcount, MPI_Datatype recvtype,
               MPI_Comm communicator) {
  /* 1. All procs send their copy of the data to one process Root (gather)
   * 2. Root proc splits its data among all the procs (broadcast)
   *
   * NOTE: we can assume sendtype == recvtype, sendcount == recvcount
   */
  int rank;
  MPI_Comm_rank(communicator, &rank);
  int size;
  MPI_Comm_size(communicator, &size);

  // root recvs all messages
  mygather(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, 0,
           communicator);
  // root sends all messages to all others
  mybcast(recvbuf, recvcount, recvtype, 0, communicator);
}
void pairwise() {}

int power(int base, int exp) {
  int temp = base;
  for (int i = 1; i < exp; i++) {
    base *= temp;
  }
  return base;
}

int main(int argc, char **argv) {
  double t1, t2;
  int *sendbuf = NULL;
  int *recvbuf = NULL;

  int rank, size;
  MPI_Init(NULL, NULL);
  MPI_Comm_size(MPI_COMM_WORLD, &size);
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);

  int msgsize = 0;

  for (int i = 5; i <= 20; i++) {
    msgsize = power(2, i);
    sendbuf = (int *)calloc(msgsize, sizeof(int));
    for (int j = 0; j < msgsize; j++) {
      sendbuf[j] = j + (msgsize * rank); // each proc gets different ints
    }
    recvbuf = (int *)calloc(msgsize * size, (sizeof(int)));

    MPI_Barrier(MPI_COMM_WORLD);
    t1 = MPI_Wtime();
    for (int n = 0; n < TIMES_TO_TEST; n++) {
      allgather(sendbuf, msgsize, MPI_INT, recvbuf, msgsize, MPI_INT,
                MPI_COMM_WORLD);
    }
    t2 = MPI_Wtime() - t1;
    MPI_Barrier(MPI_COMM_WORLD);
    MPI_Reduce(&t2, &t1, 1, MPI_DOUBLE, MPI_MAX, 0, MPI_COMM_WORLD);

    for (int m = 0; m < msgsize * size; m++) {
      assert(recvbuf[m] == m);
    }

    if (rank == 0) {
      printf("Time taken in process 0 = %g\n", t2);
      printf("Maximum time taken among all processes = %g\n", t1);
      printf("Message size was %d\n", msgsize);
    }
    free(sendbuf);
    free(recvbuf);
  }

  MPI_Finalize();
  return 0;
}
