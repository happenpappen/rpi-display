#ifndef GAMEOFLIFE_H
#define GAMEOFLIFE_H

#include "led-matrix.h"
#include "threaded-canvas-manipulator.h"

using namespace rgb_matrix;

// Conway's game of life
// Contributed by: Vliedel
class GameOfLife : public ThreadedCanvasManipulator
{
public:
  GameOfLife(Canvas *m, int delay_ms = 500, bool torus = true);
  ~GameOfLife();
  void Run();

private:
  int numAliveNeighbours(int x, int y);
  void updateValues();

  int **values_;
  int **newValues_;
  int delay_ms_;
  int r_;
  int g_;
  int b_;
  int width_;
  int height_;
  bool torus_;
};

#endif
