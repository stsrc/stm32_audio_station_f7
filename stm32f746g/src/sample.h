#ifndef _SAMPLE_H_
#define _SAMPLE_H_

#include <stdbool.h>
#include <stdint.h>

// does not have wav header
struct sample {
  int16_t *buf;
  uint32_t bytes_size;
  uint32_t samples_per_sec;
  char *name;
};

bool sample_init();

bool sample_open(const char *name);
void sample_delete(const char *name);

struct sample *sample_get(const char *name);

#endif
