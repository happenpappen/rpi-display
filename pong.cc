// -*- mode: c++; c-basic-offset: 2; indent-tabs-mode: nil; -*-
//
#include "led-matrix.h"
#include "threaded-canvas-manipulator.h"
#include "pixel-mapper.h"
#include "graphics.h"

#include <unistd.h>

#include <string>
#include <algorithm>

#include "pong.h"

using std::max;
using std::min;

using namespace rgb_matrix;

// Pong clock:
Pong::Pong(Canvas *m, int delay_ms)
    : ThreadedCanvasManipulator(m), delay_ms_(delay_ms)
{
  width_ = canvas()->width()/2;
  height_ = canvas()->height();

  xorigin_ = canvas()->width()/4;
  yorigin_ = 0;

  /*
   * Load font. This needs to be a filename with a bdf bitmap font.
   */
  if (!font.LoadFont("7x13.bdf")) {
    fprintf(stderr, "Couldn't load font '%s'\n", "7x13.bdf");
  }
  // Init values
  srand(time(NULL));
}

Pong::~Pong()
{
}

void Pong::Run()
{
  Color black = Color(0,0,0);
  Color white = Color(255,255,255);

  while (running() && !interrupt_received)
  {
    canvas()->Fill(0,0,0);

    rgb_matrix::DrawText(canvas(), font, xorigin_-16, yorigin_ + font.baseline()+2,
                           white, &black, std::to_string(Time_hour()).c_str(), 1);
    rgb_matrix::DrawText(canvas(), font, xorigin_+width_+1, yorigin_ + font.baseline()+2,
                           white, &black, std::to_string(Time_minute()).c_str(), 1);
    for (uint8_t i = Time_second() % 2; i < height_; i += 2)
    {
      canvas()->SetPixel(xorigin_ + width_ / 2, yorigin_ + i, 0, 255, 0);
    }
    this->DrawRect(xorigin_, yorigin_, width_, height_, Color(128, 128, 128));

    //if restart flag is 1, setup a new game
    if (restart)
    {
      ballpos_x = xorigin_ + width_ / 2;
      if (Time_second() == restart || 0 >= Time_second() || Time_second() >= 59)
      { // wait before restart
        ballpos_y = yorigin_ + height_ / 2;
        ballvel_x = 0;
        ballvel_y = 0;
      }
      else
      {
        //set ball start pos
        ballpos_y = randomgen(yorigin_ + 4, yorigin_ + height_ - 4);
        //pick "random" ball direction
        ballvel_x = random() % 2 ? 1 : -1;
        ballvel_y = random() % 2 ? 0.5 : -0.5;

        //reset game restart flag
        restart = 0;
      }
      // draw bats in initial positions
      bat1miss = 0;
      bat2miss = 0;
    }

    //if coming up to the minute: secs = 59 and mins < 59, flag bat 2 (right side) to miss the return so we inc the minutes score
    if (Time_second() == 59 && Time_minute() < 59)
    {
      bat1miss = 1;
    }
    // if coming up to the hour: secs = 59  and mins = 59, flag bat 1 (left side) to miss the return, so we inc the hours score.
    if (Time_second() == 59 && Time_minute() == 59)
    {
      bat2miss = 1;
    }

    //AI - we run 2 sets of 'AI' for each bat to work out where to go to hit the ball back
    //very basic AI...
    // For each bat, First just tell the bat to move to the height of the ball when we get to a random location.
    //for bat1
    if (ballpos_x == randomgen(xorigin_ + width_ / 2 + 2, xorigin_ + width_))
    {
      bat1_target_y = ballpos_y;
    }
    //for bat2
    if (ballpos_x == randomgen(xorigin_, xorigin_ + width_ / 2))
    {
      bat2_target_y = ballpos_y;
    }

    //when the ball is closer to the left bat, run the ball maths to find out where the ball will land
    if (ballpos_x == xorigin_ + width_ / 2 - 1 && ballvel_x < 0)
    {
      uint8_t end_ball_y = this->getBallEndpoint(ballpos_x, ballpos_y, ballvel_x, ballvel_y);

      //if the miss flag is set, then the bat needs to miss the ball when it gets to end_ball_y
      if (bat1miss == 1)
      {
        bat1miss = 0;
        if (end_ball_y > yorigin_ + height_ / 2)
        {
          bat1_target_y = yorigin_ + randomgen(0, 3);
        }
        else
        {
          bat1_target_y = yorigin_ + height_ - randomgen(0, 3);
        }
      }
      //if the miss flag isn't set,  set bat target to ball end point with some randomness so its not always hitting top of bat
      else
      {
        bat1_target_y = constrain(end_ball_y - randomgen(0, BAT_HEIGHT), yorigin_, yorigin_ + height_ - BAT_HEIGHT);
      }
    }

    //right bat AI
    //if positive velocity then predict for right bat - first just match ball height
    //when the ball is closer to the right bat, run the ball maths to find out where it will land
    if (ballpos_x == xorigin_ + width_ / 2 - 1 && ballvel_x > 0)
    {
      uint8_t end_ball_y = this->getBallEndpoint(ballpos_x, ballpos_y, ballvel_x, ballvel_y);

      //if flag set to miss, move bat out way of ball
      if (bat2miss == 1)
      {
        bat2miss = 0;
        //if ball end point above 8 then move bat down, else move it up- so either way it misses
        if (end_ball_y > yorigin_ + height_ / 2)
        {
          bat2_target_y = yorigin_ + randomgen(0, 3);
        }
        else
        {
          bat2_target_y = yorigin_ + height_ - randomgen(0, 3);
        }
      }
      else
      {
        //set bat target to ball end point with some randomness
        bat2_target_y = constrain(end_ball_y - randomgen(0, BAT_HEIGHT), yorigin_, yorigin_ + height_ - BAT_HEIGHT);
      }
    }

    //move bat 1 towards target
    //if bat y greater than target y move down until hit top bound (don't go any further or bat will move off screen)
    if (bat1_y > bat1_target_y && bat1_y > yorigin_)
    {
      bat1_y--;
      bat1_update = 1;
    }

    //if bat y less than target y move up until hit bottom bound
    if (bat1_y < bat1_target_y && bat1_y < yorigin_ + height_ - BAT_HEIGHT)
    {
      bat1_y++;
      bat1_update = 1;
    }

    //draw bat 1
    if (bat1_update)
    {
      this->DrawRect(xorigin_ + BAT1_X - 1, bat1_y, 2, BAT_HEIGHT, Color(0, 0, 255), true);
    }

    //move bat 2 towards target (dont go any further or bat will move off screen)
    //if bat y greater than target y move up until hit top bound
    if (bat2_y > bat2_target_y && bat2_y > yorigin_)
    {
      bat2_y--;
      bat2_update = 1;
    }

    //if bat y less than target y move up until hit bound
    if (bat2_y < bat2_target_y && bat2_y < yorigin_ + height_ - BAT_HEIGHT)
    {
      bat2_y++;
      bat2_update = 1;
    }

    //draw bat2
    if (bat2_update)
    {
      this->DrawRect(xorigin_ + BAT2_X + 1, bat2_y, 2, BAT_HEIGHT, Color(0, 0, 255), true);
    }

    //update the ball position using the velocity
    ballpos_x = ballpos_x + ballvel_x;
    ballpos_y = ballpos_y + ballvel_y;

    //check ball collision with top and bottom of screen and reverse the y velocity if either is hit
    if (ballpos_y <= yorigin_)
    {
      ballvel_y = ballvel_y * -1;
      ballpos_y = yorigin_; //make sure value goes no less that top bound
    }

    if (ballpos_y >= yorigin_ + height_ - 1)
    {
      ballvel_y = ballvel_y * -1;
      ballpos_y = yorigin_ + height_ - 1; //make sure value goes no more than 15
    }

    //check for ball collision with bat1. check ballx is same as batx
    //and also check if bally lies within width of bat i.e. baty to baty + 6. We can use the exp if(a < b && b < c)
    if ((int)ballpos_x == xorigin_ + BAT1_X + 1 && (bat1_y <= (int)ballpos_y && (int)ballpos_y <= bat1_y + BAT_HEIGHT - 1))
    {

      //random if bat flicks ball to return it - and therefor changes ball velocity
      if (!randomgen(0, 3))
      { //not true = no flick - just straight rebound and no change to ball y vel
        ballvel_x *= -1;
      }
      else
      {
        bat1_update = 1;
        uint8_t flick = 0; //0 = up, 1 = down.

        if (bat1_y > yorigin_ + 1 || bat1_y < yorigin_ + height_ / 2)
        {
          flick = randomgen(0, 2); //pick a random dir to flick - up or down
        }

        //if bat 1 or 2 away from top only flick down
        if (bat1_y <= yorigin_ + 1)
        {
          flick = 0; //move bat down 1 or 2 pixels
        }
        //if bat 1 or 2 away from bottom only flick up
        if (bat1_y >= yorigin_ + height_ / 2)
        {
          flick = 1; //move bat up 1 or 2 pixels
        }

        switch (flick)
        {
        //flick up
        case 0:
          bat1_target_y += randomgen(1, 3);
          ballvel_x *= -1;
          if (ballvel_y < 2)
          {
            ballvel_y += 0.2;
          }
          break;

        //flick down
        case 1:
          bat1_target_y -= randomgen(1, 3);
          ballvel_x *= -1;
          if (ballvel_y > 0.2)
          {
            ballvel_y -= 0.2;
          }
          break;
        }
      }
    }

    //check for ball collision with bat2. check ballx is same as batx
    //and also check if bally lies within width of bat i.e. baty to baty + 6. We can use the exp if(a < b && b < c)
    if ((int)ballpos_x == xorigin_ + BAT2_X && (bat2_y <= (int)ballpos_y && (int)ballpos_y <= bat2_y + BAT_HEIGHT - 1))
    {

      //random if bat flicks ball to return it - and therefor changes ball velocity
      if (!randomgen(0, 3))
      {
        ballvel_x *= -1; //not true = no flick - just straight rebound and no change to ball y vel
      }
      else
      {
        bat1_update = 1;
        uint8_t flick = 0; //0 = up, 1 = down.

        if (bat2_y > yorigin_ + 1 || bat2_y < yorigin_ + height_ / 2)
        {
          flick = randomgen(0, 2); //pick a random dir to flick - up or down
        }
        //if bat 1 or 2 away from top only flick down
        if (bat2_y <= yorigin_ + 1)
        {
          flick = 0; //move bat up 1 or 2 pixels
        }
        //if bat 1 or 2 away from bottom only flick up
        if (bat2_y >= yorigin_ + height_ / 2)
        {
          flick = 1; //move bat down 1 or 2 pixels
        }

        switch (flick)
        {
          //flick up
        case 0:
          bat2_target_y += randomgen(1, 3);
          ballvel_x *= -1;
          if (ballvel_y < 2)
          {
            ballvel_y += 0.2;
          }
          break;

          //flick down
        case 1:
          bat2_target_y -= randomgen(1, 3);
          ballvel_x *= -1;
          if (ballvel_y > 0.2)
          {
            ballvel_y -= 0.2;
          }
          break;
        }
      }
    }

    //plot the ball on the screen
    uint8_t plot_x = (int)(ballpos_x + 0.5f);
    uint8_t plot_y = (int)(ballpos_y + 0.5f);

    canvas()->SetPixel(plot_x, plot_y, 255, 0, 0);

    //check if a bat missed the ball. if it did, reset the game.
    if ((int)ballpos_x == xorigin_ || (int)ballpos_x == xorigin_ + width_)
    {
      restart = Time_second(); // to wait for new second before restart
      if (!restart)
        restart++;
    }

    usleep(delay_ms_ * 1000); // ms
  }
}

uint8_t Pong::getBallEndpoint(float tempballpos_x, float tempballpos_y, float tempballvel_x, float tempballvel_y)
{

  //run prediction until ball hits bat
  while (tempballpos_x > xorigin_ + BAT1_X && tempballpos_x < xorigin_ + BAT2_X)
  {
    tempballpos_x += tempballvel_x;
    tempballpos_y += tempballvel_y;
    //check for collisions with top / bottom
    if (tempballpos_y <= yorigin_ || tempballpos_y >= yorigin_ + height_ - 1)
    {
      tempballvel_y *= -1;
    }
  }
  return tempballpos_y;
}

void Pong::DrawRect(int x, int y, int width, int height, Color col, bool fill)
{
#ifdef DEBUG
  printf("DrawRect: x: %d, y: %d, w: %d, h: %d, f: %d\n", x, y, width, height, fill ? 1 : 0);
#endif

  if (!fill)
  {
    DrawLine(canvas(), x, y, x, y + height -1, col);
    DrawLine(canvas(), x, y, x + width-1, y, col);
    DrawLine(canvas(), x + width-1, y, x + width-1, y + height-1, col);
    DrawLine(canvas(), x, y + height-1, x + width-1, y + height-1, col);
  }
  else
  {
    for (int i = y; i < y + height; i++)
    {
      DrawLine(canvas(), x, i, x + width-1, i, col);
    }
  }
}

const int16_t Pong::BAT_HEIGHT = 6;
const int16_t Pong::BAT1_X = 2; // collision detection
const int16_t Pong::BAT2_X = 28;
