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
#include <iostream>

#include <mosquittopp.h>
#include "mqtt_credentials.h"

// Include demo classes:
//
#include "abeliansandpile.h"
#include "gameoflife.h"
#include "imagescroller.h"
#include "langtonsant.h"
#include "pong.h"
#include "tetrisclock.h"
#include "globals.h"

#define NOF_EFFECTS 6
#define TERM_ERR "\033[1;31m"
#define TERM_NORM "\033[0m"

//using std::max;
//using std::min;

using namespace mosqpp;
using namespace rgb_matrix;
using namespace std;

volatile bool interrupt_received = false;
volatile int running_effect = 1;
volatile int new_effect = -1;
volatile bool effect_changed = true;
const char *file_name = NULL;
const char *new_file_name = NULL;

ImageScroller *image_scroller = NULL;
ThreadedCanvasManipulator *image_gen = NULL;

// My MQTT class:
//
class MyMosquitto : public mosquittopp
{
public:
  void on_message(const struct mosquitto_message *message)
  {
    cout << "Received message on topic: " << message->topic << endl;
    cout << (char *)message->payload << endl;
    cout << "Message size: " << message->payloadlen << endl;
    if (strcmp(message->topic, "/display/set/Mode") == 0)
    {
      new_effect = atoi((const char *)message->payload);
      effect_changed = true;
    }
    if (strcmp(message->topic, "/display/set/Filename") == 0)
    {
      new_file_name = (const char *)message->payload;
      if (running_effect == 4)
      {
        if (!image_scroller->LoadPPM(new_file_name))
        {
          image_scroller->LoadPPM(file_name);
        }
        else
        {
          file_name = new_file_name;
        }
      }
    }
  };
};

static void InterruptHandler(int signo)
{
  interrupt_received = true;
}

static int usage(const char *progname)
{
  fprintf(stderr, "usage: %s <options> -D <demo-nr> [optional parameter]\n",
          progname);
  fprintf(stderr, "Options:\n");
  fprintf(stderr,
          "\t-D <demo-nr>              : Always needs to be set\n"
          "\t-t <seconds>              : Run for these number of seconds, then exit.\n");

  rgb_matrix::PrintMatrixFlags(stderr);

  fprintf(stderr, "Effects, choosen with -D\n");
  fprintf(stderr, "\t1  - Abelian sandpile model (-m <time-step-ms>)\n"
                  "\t2  - Conway's game of life (-m <time-step-ms>)\n"
                  "\t3  - Langton's ant (-m <time-step-ms>)\n");
  fprintf(stderr, "Example:\n\t%s -t 10 -D 1\n"
                  "Displays abelian sandpile animation.\n",
          progname);
  return 1;
}

int main(int argc, char *argv[])
{
  int scroll_ms = 30;
  MyMosquitto *mosq = new MyMosquitto();

  new_effect = -1;
  running_effect = -1;
  
  RGBMatrix::Options matrix_options;
  rgb_matrix::RuntimeOptions runtime_opt;

  // These are the defaults when no command-line flags are given.
  matrix_options.rows = 16;
  matrix_options.cols = 32;
  matrix_options.chain_length = 4;
  matrix_options.parallel = 1;

  // First things first: extract the command line flags that contain
  // relevant matrix options.
  if (!ParseOptionsFromFlags(&argc, &argv, &matrix_options, &runtime_opt))
  {
    return usage(argv[0]);
  }

  int opt;
  while ((opt = getopt(argc, argv, "dD:r:P:c:p:b:m:LR:")) != -1)
  {
    switch (opt)
    {
    case 'D':
      new_effect = atoi(optarg);
      running_effect = new_effect;
      break;

    case 'm':
      scroll_ms = atoi(optarg);
      break;

      // These used to be options we understood, but deprecated now. Accept
      // but don't mention in usage()
    case 'R':
      fprintf(stderr, "-R is deprecated. "
                      "Use --led-pixel-mapper=\"Rotate:%s\" instead.\n",
              optarg);
      return 1;
      break;

    case 'L':
      fprintf(stderr, "-L is deprecated. Use\n\t--led-pixel-mapper=\"U-mapper\" --led-chain=4\ninstead.\n");
      return 1;
      break;

    case 'd':
      runtime_opt.daemon = 1;
      break;

    case 'r':
      fprintf(stderr, "Instead of deprecated -r, use --led-rows=%s instead.\n",
              optarg);
      matrix_options.rows = atoi(optarg);
      break;

    case 'P':
      matrix_options.parallel = atoi(optarg);
      break;

    case 'c':
      fprintf(stderr, "Instead of deprecated -c, use --led-chain=%s instead.\n",
              optarg);
      matrix_options.chain_length = atoi(optarg);
      break;

    case 'p':
      matrix_options.pwm_bits = atoi(optarg);
      break;

    case 'b':
      matrix_options.brightness = atoi(optarg);
      break;

    default: /* '?' */
      return usage(argv[0]);
    }
  }

  if (optind < argc)
  {
    file_name = argv[optind];
  }

  if (new_effect < 0)
  {
    fprintf(stderr, TERM_ERR "Expected required option -D <demo>\n" TERM_NORM);
    return usage(argv[0]);
  }

  RGBMatrix *matrix = CreateMatrixFromOptions(matrix_options, runtime_opt);
  if (matrix == NULL)
    return 1;

  printf("Size: %dx%d. Hardware gpio mapping: %s\n",
         matrix->width(), matrix->height(), matrix_options.hardware_mapping);

  Canvas *canvas = matrix;

  // Initialize mqtt connection:
  mosq->username_pw_set(MQTT_USER, MQTT_PASSWORD);
  mosq->connect(MQTT_HOST);
  // Example for sending a message:
  // char message[] = "Hallo Welt!";
  // mosq->publish(0,"/display/status/Message", sizeof(message), message);
  cout << "Reading messages" << endl;
  mosq->subscribe(0, "/display/set/#");
  mosq->loop_start();

  // Set up an interrupt handler to be able to stop animations while they go
  // on. Note, each demo tests for while (running() && !interrupt_received) {},
  // so they exit as soon as they get a signal.
  signal(SIGTERM, InterruptHandler);
  signal(SIGINT, InterruptHandler);

  printf("Press <CTRL-C> to exit and reset LEDs\n");
  while (!interrupt_received)
  {
    if (effect_changed)
    {
      effect_changed = false;
      if (new_effect > 0 && new_effect <= NOF_EFFECTS)
      {
        running_effect = new_effect;
      }
      if (image_gen != NULL)
      {
        image_gen->Stop();
        canvas->Clear();
        delete image_gen;
        image_gen = NULL;
      }
      // The ThreadedCanvasManipulator objects are filling
      // the matrix continuously.
      switch (running_effect)
      {
      case 1:
        image_gen = new Sandpile(canvas, scroll_ms);
        break;

      case 2:
        image_gen = new GameOfLife(canvas, scroll_ms);
        break;

      case 3:
        image_gen = new Ant(canvas, scroll_ms);
        break;
      case 4:
        image_scroller = new ImageScroller(matrix, 1, scroll_ms);
        if (!image_scroller->LoadPPM(file_name))
          return -1;
        image_gen = image_scroller;
        break;
      case 5:
        image_gen = new Pong(canvas, scroll_ms);
        break;
      case 6:
        image_gen = new TetrisClock(canvas, scroll_ms);
        break;
      }
      if (image_gen == NULL)
        return usage(argv[0]);

      // Image generating demo is created. Now start the thread.
      image_gen->Start();
    }
  }

  // Stop mqtt client:
  //
  mosq->unsubscribe(0, "/display/set/#");

  // Stop image generating thread. The delete triggers
  delete image_gen;
  delete canvas;

  printf("\%s. Exiting.\n",
         interrupt_received ? "Received CTRL-C" : "Timeout reached");
  return 0;
}
