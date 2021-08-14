/*
  Name: Gregory G Danielson III
  BlazerId: gregdan3
  Course Section: CS 632
  Homework #: 3
  Instructions to compile the program:
    `gcc ./life.c -o life -std=c99 -Wall -fopenmp -Ofast`
  Instructions to run the program:
    `./life --help` or `make run`/`make display`
*/

#define _GNU_SOURCE
// #define DEBUG0
// #define DEBUG1
#include <getopt.h>
#include <omp.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
// constants for arguments
int WIDTH = 10;
int HEIGHT = 10;
int N;
int GENERATIONS = 10;
int P = 1;
int Q = 1;
static int SHOW = false;

// constants for program
const int BORDER = 2;
bool **BOARD;
bool **NEWBOARD;
bool **TEMP;

int W_BORD;
int H_BORD;
int W_BOUND;
int H_BOUND;

bool **create_2d_arr() {
  bool **ptr = (bool **)malloc(W_BORD * sizeof(bool *));
  if (ptr == NULL) {
    fprintf(stderr, "Allocating the initial board failed.\n");
  }

  for (int i = 0; i < W_BORD; i++) {
    ptr[i] = (bool *)malloc(H_BORD * sizeof(bool));
    if (ptr[i] == NULL) {
      fprintf(stderr, "Allocating the board row %d failed.\n", i);
    }
  }
  return ptr;
}

void free_2d_arr(bool **arr) {
  free(&arr[0][0]);
  free(arr);
}

void print_board() {
  // depends on prep in play_game_of_life
  // TODO: omit border of the dead?
  printf("\033[H"); // return to home i.e. upper left
  for (int x = 0; x < W_BORD; x++) {
    for (int y = 0; y < H_BORD; y++) {
      printf(BOARD[x][y] ? "\033[7m  \033[m" : "  "); // inverted tile or empty
    }
    printf("\033[E"); // newline
    fflush(stdout);
  }
};

void progress_board() {
  // depends on prep in play_game_of_life
  int x, y, neighbors;

#pragma omp parallel default(none) num_threads(P *Q)                           \
    shared(BOARD, NEWBOARD, P, Q, N) private(x, y, neighbors)
  {
    int tid = omp_get_thread_num();
    int p = tid / Q;
    int q = tid % Q;
    int myM = N / P;
    int xstart = p * myM;
    int xend = xstart + myM;
    if ((tid == 0) | (xstart == 0)) {
      xstart += 1;
    }
    if (p >= P - 1) {
      xend = N;
    }
#ifdef DEBUG1
    printf("t: [%d]\n", tid);
    printf("p: [%d/%d = %d]\n", tid, Q, p);
    printf("q: [%d%%%d = %d]\n", tid, Q, q);
    printf("m: [%d / %d = %d]\n", N, P, myM);
    printf("s: [%d * %d = %d]\n", p, myM, xstart);
    printf("e: [%d + %d = %d]\n", xstart, myM, xend);
    printf("\n");
#endif

    for (x = xstart; x < xend; x++) {
      int myN = N / Q;
      int ystart = q * myN;
      int yend = ystart + myN;
      if ((tid == 0) | (ystart == 0)) {
        ystart += 1;
      }
      if (q >= Q - 1) {
        yend = N;
      }
#ifdef DEBUG1
      printf("n: [%d / %d = %d]\n", N, Q, myN);
      printf("s: [%d * %d = %d]\n", q, myN, ystart);
      printf("e: [%d + %d = %d]\n", ystart, myN, yend);
      printf("\n");
#endif
      for (y = ystart; y < yend; y++) {
#ifdef DEBUG0
        printf("xy:(%d,%d)\n", x, y);
#endif
        /* ordered like so:
           123
           4 5
           678 */
        // TODO: reduce to 3 memory accesses with int
        neighbors = BOARD[x - 1][y - 1] + BOARD[x - 1][y] +
                    BOARD[x - 1][y + 1] + BOARD[x][y - 1] + BOARD[x][y + 1] +
                    BOARD[x + 1][y - 1] + BOARD[x + 1][y] + BOARD[x + 1][y + 1];

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
  }
  TEMP = NEWBOARD;
  NEWBOARD = BOARD;
  BOARD = TEMP;
}

void play_game_of_life() {
  W_BORD = WIDTH + BORDER;
  H_BORD = HEIGHT + BORDER;
  W_BOUND = W_BORD - 1;
  H_BOUND = H_BORD - 1;
  N = W_BOUND;

  BOARD = create_2d_arr();
  NEWBOARD = create_2d_arr();

  // fill board randomly
  for (int x = 0; x < W_BORD; x++) {
    for (int y = 0; y < H_BORD; y++) {
      if (x == 0 || y == 0 || x == W_BOUND || y == H_BOUND) {
        BOARD[x][y] = false; // border of the dead
      } else {
        BOARD[x][y] = rand() & 1;
      }
    }
  }

  for (int i = 0; i < GENERATIONS; i++) {
    if (SHOW) {
      print_board();
      usleep(200000);
    }
    progress_board();
  }
  free_2d_arr(BOARD);
  free_2d_arr(NEWBOARD);
}

int main(int argc, char **argv) {
  int c;

  int option_index = 0;
  static struct option long_options[] = {
      {"help", no_argument, 0, 'H'},
      {"show", no_argument, &SHOW, 's'},
      {"width", required_argument, 0, 'w'},
      {"height", required_argument, 0, 'h'},
      {"generations", required_argument, 0, 'g'},
  };

  while ((c = getopt_long(argc, argv, "Hw:h:g:x:y:s", long_options,
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
      printf("\t\t-x/--decomp-x: Set x dimension for board decomposition in "
             "parallelization.\n");
      printf("\t\t\t Defaults to 1.\n");
      printf("\t\t-y/--decomp-y: Set y dimension for board decomposition in "
             "parallelization.\n");
      printf("\t\t\t Defaults to 1.\n");
      printf("\t\t-s/--show: Show the simulation.\n");
      printf("\t\t\t Defaults to False.\n");
      printf("\t\t\t The simulation will run slower for viewing purposes.\n");
      printf("\t\t\t Avoid using a board larger than your viewport.\n");
      printf("\n");
      printf("\t\tExample:\n");
      printf("\t\t\t./life -h 15 -w 20 -g 10 -s\n");
      printf("\n");
      printf("\t\tExample with threading:\n");
      printf("\t\t\t./life -h 500 -w 500 -g 500 -t 16 -x 4 -y 4\n");
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
    case 'x':
      P = atoi(optarg);
      break;
    case 'y':
      Q = atoi(optarg);
      break;
    case 's':
      SHOW = true;
      break;
    }
  }

  if (WIDTH != HEIGHT) {
    printf("Inequal WIDTH and HEIGHT provided, which is not supported.\n");
    printf("Shutting down...");
    exit(1);
  }

  srand(time(NULL));
  play_game_of_life();
}
