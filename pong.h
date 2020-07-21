#ifndef PONG_H
#define PONG_H

#include "led-matrix.h"
#include "threaded-canvas-manipulator.h"
#include "utility.h"

#include "globals.h"

using namespace rgb_matrix;

// Pong clock:
class Pong : public ThreadedCanvasManipulator
{
public:
  Pong(Canvas *m, int delay_ms = 50);
  ~Pong();
  void DrawRect(int x, int y, int width, int height, Color col, bool fill=false);
  void Run();

private:
  int width_;
  int height_;
  int xorigin_;
  int yorigin_;
  int delay_ms_;

  rgb_matrix::Font font;

  uint8_t getBallEndpoint(float tempballpos_x, float  tempballpos_y, float  tempballvel_x, float tempballvel_y);

  int16_t bat1_y        = 5;   //bat starting y positions
  int16_t bat2_y        = 5;
  int16_t bat1_target_y = 5;   //bat targets for bats to move to
  int16_t bat2_target_y = 5;
  uint8_t bat1_update      = 1;   //flags - set to update bat position
  uint8_t bat2_update      = 1;
  uint8_t restart          = 1;   //game restart
  uint8_t bat1miss, bat2miss;     //flags set on the minute or hour that trigger the bats to miss the ball, thus upping the score to match the time.
  float ballpos_x, ballpos_y;
  float ballvel_x, ballvel_y;

  static const int16_t BAT_HEIGHT;
  static const int16_t BAT1_X; // collision detection
  static const int16_t BAT2_X;
};

#endif
