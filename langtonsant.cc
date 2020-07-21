// -*- mode: c++; c-basic-offset: 2; indent-tabs-mode: nil; -*-
//
// This code is public domain
// (but note, once linked against the led-matrix library, this is
// covered by the GPL v2)
//
// This is a grab-bag of various demos and not very readable.
#include "led-matrix.h"
#include "threaded-canvas-manipulator.h"
#include "pixel-mapper.h"
#include "graphics.h"

#include <unistd.h>

#include "langtonsant.h"
#include "globals.h"

using std::max;
using std::min;

#define TERM_ERR "\033[1;31m"
#define TERM_NORM "\033[0m"

using namespace rgb_matrix;

// Langton's ant
// Contributed by: Vliedel
Ant::Ant(Canvas *m, int delay_ms)
    : ThreadedCanvasManipulator(m), delay_ms_(delay_ms)
{
  numColors_ = 4;
  width_ = canvas()->width();
  height_ = canvas()->height();
  values_ = new int *[width_];
  for (int x = 0; x < width_; ++x)
  {
    values_[x] = new int[height_];
  }
}

Ant::~Ant()
{
  for (int x = 0; x < width_; ++x)
  {
    delete[] values_[x];
  }
  delete[] values_;
}

void Ant::Run()
{
  antX_ = width_ / 2;
  antY_ = height_ / 2 - 3;
  antDir_ = 0;
  for (int x = 0; x < width_; ++x)
  {
    for (int y = 0; y < height_; ++y)
    {
      values_[x][y] = 0;
      updatePixel(x, y);
    }
  }

  while (running() && !interrupt_received)
  {
    // LLRR
    switch (values_[antX_][antY_])
    {
    case 0:
    case 1:
      antDir_ = (antDir_ + 1 + 4) % 4;
      break;
    case 2:
    case 3:
      antDir_ = (antDir_ - 1 + 4) % 4;
      break;
    }

    values_[antX_][antY_] = (values_[antX_][antY_] + 1) % numColors_;
    int oldX = antX_;
    int oldY = antY_;
    switch (antDir_)
    {
    case 0:
      antX_++;
      break;
    case 1:
      antY_++;
      break;
    case 2:
      antX_--;
      break;
    case 3:
      antY_--;
      break;
    }
    updatePixel(oldX, oldY);
    if (antX_ < 0 || antX_ >= width_ || antY_ < 0 || antY_ >= height_)
      return;
    updatePixel(antX_, antY_);
    usleep(delay_ms_ * 1000);
  }
}

void Ant::updatePixel(int x, int y)
{
  switch (values_[x][y])
  {
  case 0:
    canvas()->SetPixel(x, y, 200, 0, 0);
    break;
  case 1:
    canvas()->SetPixel(x, y, 0, 200, 0);
    break;
  case 2:
    canvas()->SetPixel(x, y, 0, 0, 200);
    break;
  case 3:
    canvas()->SetPixel(x, y, 150, 100, 0);
    break;
  }
  if (x == antX_ && y == antY_)
    canvas()->SetPixel(x, y, 0, 0, 0);
}
