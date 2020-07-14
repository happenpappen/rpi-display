#ifndef LANGTONSANT_H
#define LANGTONSANT_H

#include "led-matrix.h"
#include "threaded-canvas-manipulator.h"

using namespace rgb_matrix;

// Langton's ant
// Contributed by: Vliedel
class Ant : public ThreadedCanvasManipulator
{
public:
  Ant(Canvas *m, int delay_ms = 500);
  ~Ant();
  void Run();

private:
  void updatePixel(int x, int y);

  int numColors_;
  int **values_;
  int antX_;
  int antY_;
  int antDir_; // 0 right, 1 up, 2 left, 3 down
  int delay_ms_;
  int width_;
  int height_;
};

#endif
