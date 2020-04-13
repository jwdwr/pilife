#include "led-matrix.h"
#include "threaded-canvas-manipulator.h"

#include <unistd.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

using namespace rgb_matrix;

volatile bool interrupt_received = false;
static void InterruptHandler(int signo) {
  interrupt_received = true;
}

class Life : public ThreadedCanvasManipulator {
public:
  Life(Canvas *m, int delay_ms=500)
    : ThreadedCanvasManipulator(m), delay_ms(delay_ms) {
    width = canvas()->width();
    height = canvas()->height();

    grid = new int*[width];
    nextGrid = new int*[width];

    srand(time(NULL));
    for (int x=0; x<width; x++) {
      grid[x] = new int[height];
      nextGrid[x] = new int[height];
      for (int y=0; y<height; y++) {
        grid[x][y] = floor((rand() % 4) * 0.34);
      }
    }
  }

  ~Life() {
    for (int x=0; x<width; x++) {
      delete[] grid[x];
      delete[] nextGrid[x];
    }

    delete[] grid;
    delete[] nextGrid;
  }

  void Run() {
    while (running() && !interrupt_received) {
      update();
      draw();
      usleep(delay_ms * 1000);
    }
  }

private:
  void update() {
    for (int x=0; x<width; x++) {
      for (int y=0; y<height; y++) {
        int neighbors = countNeighbors(x, y);

        if (neighbors == 3 || (grid[x][y] == 1 && neighbors == 2)) {
          nextGrid[x][y] = 1;
        } else {
          nextGrid[x][y] = 0;
        }
      }
    }

    for (int x=0; x<width; x++) {
      for (int y=0; y<height; y++) {
        grid[x][y] = nextGrid[x][y];
      }
    }
  }

  int countNeighbors(int homeX, int homeY) {
    int count = 0;

    for (int x = homeX - 1; x <= homeX + 1; x++) {
      for (int y = homeY - 1; y <= homeY + 1; y++) {
        if ((x == homeX && y == homeY)
            || x < 0 || x >= width
            || y < 0 || y >= height) {
          continue;
        } else if (grid[x][y] > 0) {
          count++;
        }
      }
    }

    return count;
  }

  void draw() {
    for (int x=0; x<width; x++) {
      for (int y=0; y<height; y++) {
        if (grid[x][y] == 0) {
          canvas()->SetPixel(x, y, 0, 0, 0);
        } else {
          canvas()->SetPixel(x, y, 255, 0, 0);
        }
      }
    }
  }

  int delay_ms;
  int width;
  int height;
  int** grid;
  int** nextGrid;
};


int main(int argc, char *argv[]) {
  signal(SIGTERM, InterruptHandler);
  signal(SIGINT, InterruptHandler);

  RGBMatrix::Options matrix_options;
  rgb_matrix::RuntimeOptions runtime_opt;

  // These are the defaults when no command-line flags are given.
  matrix_options.rows = 16;
  matrix_options.chain_length = 1;
  matrix_options.parallel = 1;

  ParseOptionsFromFlags(&argc, &argv, &matrix_options, &runtime_opt);

  RGBMatrix *matrix = CreateMatrixFromOptions(matrix_options, runtime_opt);

  if (matrix == NULL)
    return 1;

  Canvas *canvas = matrix;
  ThreadedCanvasManipulator *life = new Life(canvas);
  life->Start();

  while (!interrupt_received) {
    sleep(1); // Time doesn't really matter. The syscall will be interrupted.
  }

  // Stop image generating thread. The delete triggers
  delete life;
  delete matrix;

  printf("\%s. Exiting.\n",
         interrupt_received ? "Received CTRL-C" : "Timeout reached");
  return 0;
}