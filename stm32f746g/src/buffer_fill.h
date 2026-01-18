#ifndef _BUFFER_FILL_H_
#define _BUFFER_FILL_H_
#include <stdint.h>

#include "sample.h"

struct audio_sample {
  struct sample *sample;
  uint32_t start_sample; // in samples (uint16_t) in 4 bars
  uint32_t stop_sample;  // in samples (uint16_t) in 4 bars
  struct audio_sample *next;
};

void fill_buffer(struct audio_sample *head, int16_t *buf_start,
                 int16_t *buf_end, const uint32_t start_gap,
                 const uint32_t end_gap, const uint32_t end);

void fill_buffer_with_sample(struct audio_sample *add, int16_t *buf_start,
                             int16_t *buf_end, const uint32_t start_gap,
                             const uint32_t end_gap, const uint32_t end);
#endif
