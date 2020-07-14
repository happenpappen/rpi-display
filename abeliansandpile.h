#ifndef ABELIANSANDPILE_H
#define ABELIANSANDPILE_H

#include "led-matrix.h"
#include "threaded-canvas-manipulator.h"

#include "globals.h"

using namespace rgb_matrix;

// Abelian sandpile
// Contributed by: Vliedel
class Sandpile : public ThreadedCanvasManipulator
{
public:
  Sandpile(Canvas *m, int delay_ms = 50);
  ~Sandpile();
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
