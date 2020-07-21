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
#include <unistd.h>

#include "imagescroller.h"
#include "globals.h"

using std::max;
using std::min;

using namespace rgb_matrix;

#define EXIT_WITH_MSG(m)                              \
  {                                                   \
    fprintf(stderr, "%s: %s |%s", filename, m, line); \
    fclose(f);                                        \
    return false;                                     \
  }

// Scroll image with "scroll_jumps" pixels every "scroll_ms" milliseconds.
// If "scroll_ms" is negative, don't do any scrolling.
ImageScroller::ImageScroller(RGBMatrix *m, int scroll_jumps, int scroll_ms)
    : ThreadedCanvasManipulator(m), scroll_jumps_(scroll_jumps),
      scroll_ms_(scroll_ms),
      horizontal_position_(0),
      matrix_(m)
{
  offscreen_ = matrix_->CreateFrameCanvas();
}

ImageScroller::~ImageScroller()
{
  Stop();
  WaitStopped(); // only now it is safe to delete our instance variables.
}

// _very_ simplified. Can only read binary P6 PPM. Expects newlines in headers
// Not really robust. Use at your own risk :)
// This allows reload of an image while things are running, e.g. you can
// live-update the content.
bool ImageScroller::LoadPPM(const char *filename)
{
  FILE *f = fopen(filename, "r");
  // check if file exists
  if (f == NULL && access(filename, F_OK) == -1)
  {
    fprintf(stderr, "File \"%s\" doesn't exist\n", filename);
    return false;
  }
  if (f == NULL)
    return false;

  char header_buf[256];
  const char *line = ReadLine(f, header_buf, sizeof(header_buf));

  if (sscanf(line, "P6 ") == EOF)
    EXIT_WITH_MSG("Can only handle P6 as PPM type.");

  line = ReadLine(f, header_buf, sizeof(header_buf));
  int new_width, new_height;
  if (!line || sscanf(line, "%d %d ", &new_width, &new_height) != 2)
    EXIT_WITH_MSG("Width/height expected");

  int value;
  line = ReadLine(f, header_buf, sizeof(header_buf));
  if (!line || sscanf(line, "%d ", &value) != 1 || value != 255)
    EXIT_WITH_MSG("Only 255 for maxval allowed.");

  const size_t pixel_count = new_width * new_height;
  Pixel *new_image = new Pixel[pixel_count];
  assert(sizeof(Pixel) == 3); // we make that assumption.
  if (fread(new_image, sizeof(Pixel), pixel_count, f) != pixel_count)
  {
    line = "";
    EXIT_WITH_MSG("Not enough pixels read.");
  }
  fclose(f);

  fprintf(stderr, "Read image '%s' with %dx%d\n", filename,
          new_width, new_height);

  horizontal_position_ = 0;
  MutexLock l(&mutex_new_image_);
  new_image_.Delete(); // in case we reload faster than is picked up
  new_image_.image = new_image;
  new_image_.width = new_width;
  new_image_.height = new_height;
  return true;
}

void ImageScroller::Run()
{
  const int screen_height = offscreen_->height();
  const int screen_width = offscreen_->width();
  while (running() && !interrupt_received)
  {
    {
      MutexLock l(&mutex_new_image_);
      if (new_image_.IsValid())
      {
        current_image_.Delete();
        current_image_ = new_image_;
        new_image_.Reset();
      }
    }
    if (!current_image_.IsValid())
    {
      usleep(100 * 1000);
      continue;
    }
    for (int x = 0; x < screen_width; ++x)
    {
      for (int y = 0; y < screen_height; ++y)
      {
        const Pixel &p = current_image_.getPixel(
            (horizontal_position_ + x) % current_image_.width, y);
        offscreen_->SetPixel(x, y, p.red, p.green, p.blue);
      }
    }
    offscreen_ = matrix_->SwapOnVSync(offscreen_);
    horizontal_position_ += scroll_jumps_;
    if (horizontal_position_ < 0)
      horizontal_position_ = current_image_.width;
    if (scroll_ms_ <= 0)
    {
      // No scrolling. We don't need the image anymore.
      current_image_.Delete();
    }
    else
    {
      usleep(scroll_ms_ * 1000);
    }
  }
}

// Read line, skip comments.
char *ImageScroller::ReadLine(FILE *f, char *buffer, size_t len)
{
  char *result;
  do
  {
    result = fgets(buffer, len, f);
  } while (result != NULL && result[0] == '#');
  return result;
}
