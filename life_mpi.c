#define _GNU_SOURCE
#include <getopt.h>
#include <mpi.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
// constants for arguments
int WIDTH = 10;
int HEIGHT = 10;
int GENERATIONS = 10;
static int SHOW = false;
static int NOBLOCK = false;

// constants for program
const int BORDER = 2;
bool **BOARD;
bool **NEWBOARD;
bool **TEMP;

int W_BORD;
int H_BORD;
int W_BOUND;
int H_BOUND;

bool **create_2d_arr(int rank, int size) {
  // every instance has a 0 -> HEIGHT / size # of rows
  // but a constant number of columns
  int local_rows_b = (HEIGHT / size) + BORDER;

  if (rank == (size - 1)) {
    local_rows_b += HEIGHT % size;
  }

  bool *board = (bool *)malloc(local_rows_b * W_BORD * sizeof(bool));
  bool **array = (bool **)malloc(local_rows_b * sizeof(bool *));
  if (board == NULL || array == NULL) {
    fprintf(stderr, "Allocating the initial board failed.\n");
  }
  for (int i = 0; i < local_rows_b; i++) {
    array[i] = &board[i * W_BORD];
  }
  return array;
}

void free_2d_arr(bool **arr, int rank, int size) {
  free(&arr[0][0]);
  free(arr);
}

void print_subsection(int rank, int size) {
  int local_rows_b = (HEIGHT / size) + BORDER;
  int xstart, xend;
  if (rank == 0) {
    xstart = 0;
  } else { // skip in-proc border
    xstart = 1;
  }
  if (rank == size - 1) {
    xend = local_rows_b;
  } else { // skip in-proc border
    xend = local_rows_b - 1;
  }
  for (int x = xstart; x < xend; x++) {
    for (int y = 0; y < H_BORD; y++) {
      printf(BOARD[x][y] ? "\033[7m  \033[m" : "  "); // inverted tile or empty
    }
    printf("\033[E"); // newline
  }
}

void print_board(int rank, int size) {
  // depends on prep in play_game_of_life
  // TODO: omit border of the dead?
  if (rank == 0) {
    printf("\033[H"); // return to home i.e. upper left
  }
  for (int i = 0; i < size; i++) {
    if (i == rank) {
      print_subsection(rank, size);
    }
    MPI_Barrier(MPI_COMM_WORLD);
  }
  fflush(stdout);
}

void progress_board(int rank, int size) {
  // depends on prep in play_game_of_life
  MPI_Request sreq1, sreq2, rreq1, rreq2; // objs specific to noblock
  MPI_Status recv_stat, send_stat;
  int x, y, neighbors, upper_board, lower_board;
  int local_rows_b = (HEIGHT / size) + BORDER;

  if (rank == (size - 1)) {
    local_rows_b += HEIGHT % size;
  }
  upper_board = rank - 1;
  lower_board = rank + 1;
  if (rank == 0) // no upper board
    upper_board = MPI_PROC_NULL;
  if (rank == (size - 1)) // no lower board
    lower_board = MPI_PROC_NULL;

  if (NOBLOCK) {
    MPI_Isend(BOARD[1], W_BORD, MPI_C_BOOL, upper_board, 0, MPI_COMM_WORLD,
              &sreq1);
    MPI_Irecv(BOARD[local_rows_b - 1], W_BORD, MPI_C_BOOL, lower_board, 0,
              MPI_COMM_WORLD, &rreq1);
    MPI_Isend(BOARD[local_rows_b - 2], W_BORD, MPI_C_BOOL, lower_board, 1,
              MPI_COMM_WORLD, &sreq2);
    MPI_Irecv(BOARD[0], W_BORD, MPI_C_BOOL, upper_board, 1, MPI_COMM_WORLD,
              &rreq2);
    MPI_Wait(&sreq1, &send_stat);
    MPI_Wait(&rreq1, &recv_stat);
    MPI_Wait(&sreq2, &send_stat);
    MPI_Wait(&rreq2, &recv_stat);
  } else {
    MPI_Sendrecv(BOARD[1], W_BORD, MPI_C_BOOL, upper_board, 0,
                 BOARD[local_rows_b - 1], W_BORD, MPI_C_BOOL, lower_board, 0,
                 MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    MPI_Sendrecv(BOARD[local_rows_b - 2], W_BORD, MPI_C_BOOL, lower_board, 1,
                 BOARD[0], W_BORD, MPI_C_BOOL, upper_board, 1, MPI_COMM_WORLD,
                 MPI_STATUS_IGNORE);
  }

  for (x = 1; x < local_rows_b - 1; x++) {
    for (y = 1; y < W_BOUND; y++) {
      /* ordered like so:
         123
         4 5
         678 */
      // TODO: reduce to 3 memory accesses with int
      neighbors = BOARD[x - 1][y - 1] + BOARD[x - 1][y] + BOARD[x - 1][y + 1] +
                  BOARD[x][y - 1] + BOARD[x][y + 1] + BOARD[x + 1][y - 1] +
                  BOARD[x + 1][y] + BOARD[x + 1][y + 1];

      if (BOARD[x][y]) { // living cell rules
        if (neighbors == 2 || neighbors == 3) {
          NEWBOARD[x][y] = true; // remains alive
        } else {
          NEWBOARD[x][y] = false; // dead, over or under populated
        }
      } else { // dead cell rules
        if (neighbors == 3) {
          NEWBOARD[x][y] = true; // becomes alive
        } else {
          NEWBOARD[x][y] = false; // remains dead
        }
      }
    }
  }
  TEMP = NEWBOARD;
  NEWBOARD = BOARD;
  BOARD = TEMP;
}

void play_game_of_life(int rank, int size) {
  BOARD = create_2d_arr(rank, size);
  NEWBOARD = create_2d_arr(rank, size);
  int local_rows = (HEIGHT / size);
  int local_rows_b = local_rows + BORDER;

  if (rank == (size - 1)) {
    local_rows_b += HEIGHT % size;
  }

  // consume random calls based on rank to sync
  int to_consume = (rank * local_rows * WIDTH);
  while (to_consume > 0) {
    rand();
    to_consume--;
  }

  // fill board randomly
  for (int x = 0; x < local_rows_b; x++) {
    for (int y = 0; y < W_BORD; y++) {
      if (x == 0 || y == 0 || x == local_rows_b - 1 || y == H_BOUND) {
        BOARD[x][y] = false; // border of the dead
      } else {
        BOARD[x][y] = rand() & 1;
      } // 1st loop sends borders before calc; no need to send/recv
      NEWBOARD[x][y] = false; // guarantee newboard is empty
    }
  }

  for (int i = 0; i < GENERATIONS; i++) {
    if (SHOW) {
      print_board(rank, size);
      usleep(200000);
    }
    progress_board(rank, size);
  }
  free_2d_arr(BOARD, rank, size);
  free_2d_arr(NEWBOARD, rank, size);
}

int main(int argc, char **argv) {
  int c;
  int option_index = 0;
  static struct option long_options[] = {
      {"help", no_argument, 0, 'H'},
      {"noblock", no_argument, &NOBLOCK, 'n'},
      {"show", no_argument, &SHOW, 's'},
      {"width", required_argument, 0, 'w'},
      {"height", required_argument, 0, 'h'},
      {"generations", required_argument, 0, 'g'},
  };

  while ((c = getopt_long(argc, argv, "Hw:h:g:sn", long_options,
                          &option_index)) != -1) {
    switch (c) {
    case 'H':
      printf("\n");
      printf("\tConway's Game of Life in C\n");

      printf("\t\t-H/--help: See this dialog and exit.\n");

      printf("\t\t-h/--height: Set the height of the simulation.\n");
      printf("\t\t\t Defaults to 10.\n");

      printf("\t\t-w/--width: Set the width of the simulation.\n");
      printf("\t\t\t Defaults to 10.\n");

      printf("\t\t-g/--generations: Set how many generations to simulate.\n");
      printf("\t\t\t Defaults to 10.\n");

      printf("\t\t-s/--show: Show the simulation.\n");
      printf("\t\t\t Defaults to False.\n");
      printf("\t\t\t The simulation will run slower for viewing purposes.\n");
      printf("\t\t\t Avoid using a board larger than your viewport.\n");
      printf("\t\t\t WARNING: Unstable!\n");

      printf("\t\t-n/--noblock: Use non-blocking MPI calls.\n");
      printf("\t\t\t Defaults to False.\n");

      printf("\n");

      printf("\t\tExample:\n");
      printf("\t\t\t./life -h 15 -w 20 -g 10 -s\n");
      printf("\n");
      exit(0);
    case 'w':
      WIDTH = atoi(optarg);
      break;
    case 'h':
      HEIGHT = atoi(optarg);
      break;
    case 'g':
      GENERATIONS = atoi(optarg);
      break;
    case 's':
      SHOW = true;
      break;
    case 'n':
      NOBLOCK = true;
      break;
    }
  }

  W_BORD = WIDTH + BORDER;
  H_BORD = HEIGHT + BORDER;
  W_BOUND = W_BORD - 1;
  H_BOUND = H_BORD - 1;

  int rank, size;
  MPI_Init(NULL, NULL);
  MPI_Comm_size(MPI_COMM_WORLD, &size);
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);

  srand(time(NULL));

  play_game_of_life(rank, size);
  MPI_Finalize();
}
