#define _GNU_SOURCE
#include <getopt.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

const int border = 2;
bool **create_2d_arr(int w, int h) {
  bool **ptr = (bool **)malloc(w * sizeof(bool *));
  if (ptr == NULL) {
    fprintf(stderr, "Allocating the initial board failed.\n");
  }

  for (int i = 0; i < w; i++) {
    ptr[i] = (bool *)malloc(h * sizeof(bool));
    if (ptr[i] == NULL) {
      fprintf(stderr, "Allocating the board row %d failed.\n", i);
    }
  }
  return ptr;
}

void print_board(void *b, int w, int h) {
  // expects full bounds of whole board
  // TODO: omit border of the dead?
  bool(**board) = b;
  // bool(*board)[w] = b;
  printf("\033[H"); // return to home i.e. upper left
  for (int x = 0; x < w; x++) {
    for (int y = 0; y < h; y++) {
      printf(board[x][y] ? "\033[7m  \033[m" : "  "); // inverted tile or empty
    }
    printf("\033[E"); // newline
    fflush(stdout);
  }
};

void progress_board(void *b, void *n, int w, int h) {
  // expects full bounds of whole board
  bool(**board) = b;
  bool(**new) = n;
  int neighbors;
  for (int x = 1; x < w - 1; x++) {
    for (int y = 1; y < h - 1; y++) {
      /* ordered like so:
         123
         4 5
         678 */
      // TODO: reduce to 3 memory accesses with int, hahahhaha
      neighbors = board[x - 1][y - 1] + board[x - 1][y] + board[x - 1][y + 1] +
                  board[x][y - 1] + board[x][y + 1] + board[x + 1][y - 1] +
                  board[x + 1][y] + board[x + 1][y + 1];
      if (board[x][y]) { // living cell rules
        if (neighbors == 2 || neighbors == 3) {
          new[x][y] = true; // remains alive
        } else {
          new[x][y] = false; // dead, over or under populated
        }
      } else { // dead cell rules
        if (neighbors == 3) {
          new[x][y] = true; // becomes alive
        } else {
          new[x][y] = false; // remains dead
        }
      }
    }
  }

  for (int x = 1; x < w - 1; x++) { // don't replace border of the dead
    for (int y = 1; y < h - 1; y++) {
      board[x][y] = new[x][y];
    }
  }
}

void play_game_of_life(int w, int h, int gens, bool show) {
  w = w + border;
  h = h + border;
  int w_bound = w - 1;
  int h_bound = h - 1;

  bool **board = create_2d_arr(w, h);
  bool **newboard = create_2d_arr(w, h);

  // fill board randomly
  for (int x = 0; x < w; x++) {
    for (int y = 0; y < h; y++) {
      if (x == 0 || y == 0 || x == w_bound || y == h_bound) {
        board[x][y] = false; // border of the dead
      } else {
        board[x][y] = rand() & 1;
      }
    }
  }

  for (int i = 0; i < gens; i++) {
    if (show) {
      print_board(board, w, h);
      usleep(200000);
    }
    progress_board(board, newboard, w, h);
  }
  free(board);
  free(newboard);
}

int main(int argc, char **argv) {
  int c;
  int width = 10;
  int height = 10;
  int generations = 10;
  static int show = false;

  int option_index = 0;
  static struct option long_options[] = {
      {"help", no_argument, 0, 'H'},
      {"show", no_argument, &show, 's'},
      {"width", required_argument, 0, 'w'},
      {"height", required_argument, 0, 'h'},
      {"generations", required_argument, 0, 'g'},
  };

  while ((c = getopt_long(argc, argv, "Hw:h:g:s", long_options,
                          &option_index)) != -1) {
    switch (c) {
    case 'H':
      printf("\n\tConway's Game of Life in C\n");
      printf("\t\t-H/--help: See this dialog and exit.\n");
      printf("\t\t-h/--height: Set the height of the simulation.\n");
      printf("\t\t\t Defaults to 10.\n");
      printf("\t\t-w/--width: Set the width of the simulation.\n");
      printf("\t\t\t Defaults to 10.\n");
      printf("\t\t-g/--generations: Set how many generations to simulate.\n");
      printf("\t\t\t Defaults to 10.\n");
      printf("\t\t-s/--show: Show the simulation. Use a small board.\n");
      printf("\t\t\t Defaults to False. Do not use an overly large board!\n");
      printf("\t\t\t The simulation will run slower for viewing purposes.\n");
      printf("\n");
      printf("\t\tExample execution: ./life -h 15 -w 20 -g 10 -s\n");
      printf("\t\tNOTE: There is no delay in\n");
      printf("\n");
      exit(0);
    case 'w':
      width = atoi(optarg);
      break;
    case 'h':
      height = atoi(optarg);
      break;
    case 'g':
      generations = atoi(optarg);
      break;
    case 's':
      show = true;
      break;
    }
  }

  // TODO: warn user about long runtimes?
  // TODO: newlines?
  // if (show && generations > 100) {
  //   printf("WARNING: You have chose to show the board for a large number of "
  //          "generations. This will take a long time! Are you sure you want to
  //          " "continue?");
  // }

  srand(time(NULL));
  play_game_of_life(width, height, generations, show);
}
