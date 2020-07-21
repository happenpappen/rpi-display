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

#include <assert.h>
#include <getopt.h>
#include <limits.h>
#include <math.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <algorithm>

#include "tetrisclock.h"

using std::max;
using std::min;

using namespace rgb_matrix;

// Abelian sandpile
// Contributed by: Vliedel
TetrisClock::TetrisClock(Canvas *m, int delay_ms)
    : ThreadedCanvasManipulator(m), delay_ms_(delay_ms)
{
  width_ = canvas()->width() - 1;   // We need an odd width
  height_ = canvas()->height() - 1; // We need an odd height

  // Allocate memory
  values_ = new int *[width_];
  for (int x = 0; x < width_; ++x)
  {
    values_[x] = new int[height_];
  }
  newValues_ = new int *[width_];
  for (int x = 0; x < width_; ++x)
  {
    newValues_[x] = new int[height_];
  }

  // Init values
  srand(time(NULL));
  for (int x = 0; x < width_; ++x)
  {
    for (int y = 0; y < height_; ++y)
    {
      values_[x][y] = 0;
    }
  }
}

TetrisClock::~TetrisClock()
{
  for (int x = 0; x < width_; ++x)
  {
    delete[] values_[x];
  }
  delete[] values_;
  for (int x = 0; x < width_; ++x)
  {
    delete[] newValues_[x];
  }
  delete[] newValues_;
}

void TetrisClock::Run()
{
  while (running() && !interrupt_received)
  {
    // Drop a sand grain in the centre
    values_[width_ / 2][height_ / 2]++;
    updateValues();

    for (int x = 0; x < width_; ++x)
    {
      for (int y = 0; y < height_; ++y)
      {
        switch (values_[x][y])
        {
        case 0:
          canvas()->SetPixel(x, y, 0, 0, 0);
          break;
        case 1:
          canvas()->SetPixel(x, y, 0, 0, 200);
          break;
        case 2:
          canvas()->SetPixel(x, y, 0, 200, 0);
          break;
        case 3:
          canvas()->SetPixel(x, y, 150, 100, 0);
          break;
        default:
          canvas()->SetPixel(x, y, 200, 0, 0);
        }
      }
    }
    usleep(delay_ms_ * 1000); // ms
  }
}

void TetrisClock::updateValues()
{
  // Copy values to newValues
  for (int x = 0; x < width_; ++x)
  {
    for (int y = 0; y < height_; ++y)
    {
      newValues_[x][y] = values_[x][y];
    }
  }

  // Update newValues based on values
  for (int x = 0; x < width_; ++x)
  {
    for (int y = 0; y < height_; ++y)
    {
      if (values_[x][y] > 3)
      {
        // Collapse
        if (x > 0)
          newValues_[x - 1][y]++;
        if (x < width_ - 1)
          newValues_[x + 1][y]++;
        if (y > 0)
          newValues_[x][y - 1]++;
        if (y < height_ - 1)
          newValues_[x][y + 1]++;
        newValues_[x][y] -= 4;
      }
    }
  }
  // Copy newValues to values
  for (int x = 0; x < width_; ++x)
  {
    for (int y = 0; y < height_; ++y)
    {
      values_[x][y] = newValues_[x][y];
    }
  }
}
