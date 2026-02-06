#ifndef _PLAY_H_
#define _PLAY_H_

#include <cmsis_os.h>
#include <stdint.h>

#include "sample.h"

extern osMessageQId Play_MessageId;

struct play {
  uint32_t BPM;
  uint32_t samples_per_sec;
  bool metronome_enabled;
  char *metronome_name;
};

enum play_op {
  SETUP = 0,
  PAUSE,
  RESUME,
  STOP,
  DELETE_ALL,
  ADD,
  FILL_BUFFER,
  METRONOME,
};

struct play_message {
  enum play_op op;
  union {
    struct play settings;
    const char *name;
    bool first_half;
  } data;
};

void play_thread(void const *arg);
#endif
