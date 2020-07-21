#ifndef TETRISCLOCK_H
#define TETRISCLOCK_H

#include "led-matrix.h"
#include "threaded-canvas-manipulator.h"

#include "globals.h"

using namespace rgb_matrix;

// Tetrisclock class:
class TetrisClock : public ThreadedCanvasManipulator
{
public:
  TetrisClock(Canvas *m, int delay_ms = 50);
  ~TetrisClock();
  void Run();

private:
  void updateValues();

  int width_;
  int height_;
  int **values_;
  int **newValues_;
  int delay_ms_;
};

#endif
