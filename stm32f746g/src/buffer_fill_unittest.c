#include "buffer_fill.h"

#include <unity.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

void setUp(void) {
  // runs before each test
}

void tearDown(void) {
  // runs after each test
}

int32_t _mix_val(int16_t buf, int16_t sample) {
  int32_t mix = buf + sample;
  if (mix != (int16_t)mix) {
    if (mix > INT16_MAX) {
      mix = INT16_MAX;
    } else {
      mix = INT16_MIN;
    }
  }
  return mix;
}

#define mix_val(x, y) _mix_val(x << 8 | x, y << 8 | y)

void test_fill_simple_case(void) {
  struct audio_sample head = {0};
  struct sample sample = {0};
  const int sample_size = 32;
  sample.buf = malloc(sample_size);
  const uint8_t sample_byte = 0x44;
  memset(sample.buf, sample_byte, sample_size);
  sample.bytes_size = sample_size;
  sample.samples_per_sec = 44100;
  sample.name = strdup("A.wav");
  head.sample = &sample;
  head.start_sample = 0;
  head.stop_sample = head.start_sample + sample_size / sizeof(uint16_t);
  const int buf_size = 512;
  int16_t *buf = malloc(buf_size);
  const uint8_t buf_byte = 0x22;
  int32_t mix = mix_val(buf_byte, sample_byte);
  memset(buf, buf_byte, buf_size);

  fill_buffer(&head, buf, buf + (buf_size / sizeof(uint16_t)), 0, 256, 1024);
  for (int i = 0; i < 32 / sizeof(uint16_t); i++) {
    TEST_ASSERT_EQUAL(mix, buf[i]);
  }
  for (int i = 32 / sizeof(uint16_t); i < 512 / sizeof(uint16_t); i++) {
    TEST_ASSERT_EQUAL(buf_byte << 8 | buf_byte, buf[i]);
  }
  memset(buf, buf_byte, buf_size);

  fill_buffer(&head, buf, buf + (buf_size / sizeof(uint16_t)), 8, 256, 1024);
  for (int i = 0; i < 16 / sizeof(uint16_t); i++) {
    TEST_ASSERT_EQUAL(mix, buf[i]);
  }
  for (int i = 16 / sizeof(uint16_t); i < 512 / sizeof(uint16_t); i++) {
    TEST_ASSERT_EQUAL(buf_byte << 8 | buf_byte, buf[i]);
  }
  memset(buf, buf_byte, buf_size);

  fill_buffer(&head, buf, buf + (buf_size / sizeof(uint16_t)), 16, 256, 1024);
  for (int i = 0 / sizeof(uint16_t); i < 512 / sizeof(uint16_t); i++) {
    TEST_ASSERT_EQUAL(buf_byte << 8 | buf_byte, buf[i]);
  }
  memset(buf, buf_byte, buf_size);

  fill_buffer(&head, buf, buf + (buf_size / sizeof(uint16_t)), 1000, 64, 1024);
  for (int i = 0 / sizeof(uint16_t); i < 48 / sizeof(uint16_t); i++) {
    TEST_ASSERT_EQUAL(buf_byte << 8 | buf_byte, buf[i]);
  }
  for (int i = 48 / sizeof(uint16_t); i < (48 + 32) / sizeof(uint16_t); i++) {
    TEST_ASSERT_EQUAL(mix, buf[i]);
  }
  for (int i = (48 + 32) / sizeof(uint16_t); i < (48 + 64) / sizeof(uint16_t);
       i++) {
    TEST_ASSERT_EQUAL(buf_byte << 8 | buf_byte, buf[i]);
  }
  free(sample.name);
  free(sample.buf);
  free(buf);
}

static void init_audio_sample(struct audio_sample *audio_sample,
                              uint32_t bytes_size, uint8_t value,
                              const char *name, uint32_t start, uint32_t end) {
  struct sample *sample = calloc(1, sizeof(struct sample));
  audio_sample->sample = sample;
  sample->buf = malloc(bytes_size);
  memset(sample->buf, value, bytes_size);
  sample->bytes_size = bytes_size;
  sample->samples_per_sec = 44100;
  sample->name = strdup(name);
  audio_sample->start_sample = start;
  audio_sample->stop_sample = (start + bytes_size / sizeof(int16_t)) % end;
}

static void deinit_audio_sample(struct audio_sample *sample) {
  free(sample->sample->name);
  free(sample->sample->buf);
  free(sample->sample);
}

void test_fill_multiple_samples_simple_case(void) {
  struct audio_sample audio_samples[3] = {0};
  audio_samples[0].next = &audio_samples[1];
  audio_samples[1].next = &audio_samples[2];

  const uint32_t end = 1024;
  init_audio_sample(&audio_samples[0], 32, 0x44, "A.wav", 0, end);
  init_audio_sample(&audio_samples[1], 64, 0x66, "B.wav", 2, end);
  init_audio_sample(&audio_samples[2], 128, 0x88, "C.wav", 4, end);

  const int buf_size = 512;
  int16_t *buf = calloc(1, buf_size);
  int32_t value = mix_val(0, 0x44);

  fill_buffer(audio_samples, buf, buf + (buf_size / sizeof(uint16_t)), 0, 256,
              end);
  for (int i = 0; i < 4 / sizeof(uint16_t); i++) {
    TEST_ASSERT_EQUAL(value, buf[i]);
  }
  value = mix_val(0x44, 0x66);
  for (int i = 4 / sizeof(uint16_t); i < 8 / sizeof(uint16_t); i++) {
    TEST_ASSERT_EQUAL(value, buf[i]);
  }

  value = _mix_val((int16_t)value, 0x88 << 8 | 0x88);
  for (int i = 8 / sizeof(int16_t); i < 32 / sizeof(int16_t); i++) {
    TEST_ASSERT_EQUAL(value, buf[i]);
  }

  value = mix_val(0x66, 0x88);
  for (int i = 32 / sizeof(int16_t); i < 68 / sizeof(int16_t); i++) {
    TEST_ASSERT_EQUAL(value, buf[i]);
  }

  value = mix_val(0, 0x88);
  for (int i = 68 / sizeof(int16_t); i < 136 / sizeof(int16_t); i++) {
    TEST_ASSERT_EQUAL(value, buf[i]);
  }

  for (int i = 136 / sizeof(int16_t); i < 256 / sizeof(int16_t); i++) {
    TEST_ASSERT_EQUAL(0, buf[i]);
  }

  memset(buf, 0, buf_size);
  fill_buffer(audio_samples, buf, buf + (buf_size / sizeof(int16_t)), 0, 34,
              end);

  value = mix_val(0x00, 0x44);
  for (int i = 0; i < 4 / sizeof(uint16_t); i++) {
    TEST_ASSERT_EQUAL(value, buf[i]);
  }

  value = mix_val(0x44, 0x66);
  for (int i = 4 / sizeof(uint16_t); i < 8 / sizeof(uint16_t); i++) {
    TEST_ASSERT_EQUAL(value, buf[i]);
  }

  value = _mix_val(value, 0x88 << 8 | 0x88);
  for (int i = 8 / sizeof(int16_t); i < 32 / sizeof(int16_t); i++) {
    TEST_ASSERT_EQUAL(value, buf[i]);
  }

  value = mix_val(0x66, 0x88);
  for (int i = 32 / sizeof(int16_t); i < 68 / sizeof(int16_t); i++) {
    TEST_ASSERT_EQUAL(value, buf[i]);
  }

  for (int i = 68 / sizeof(int16_t); i < 256 / sizeof(int16_t); i++) {
    TEST_ASSERT_EQUAL(0, buf[i]);
  }

  for (int i = 0; i < 3; i++) {
    deinit_audio_sample(&audio_samples[i]);
  }
  free(buf);
}

void test_problem(void) {
  const uint32_t end = 235200;
  const uint8_t val = 0x11;
  const char *name = "A.wav";
  const uint32_t length = 110432;
  uint32_t start_times[] = {35097, 96687, 154308, 202668, 18517, 4945};
  struct audio_sample audio_samples[sizeof(start_times) / sizeof(uint32_t)] = {
      0};
  /*
   * 35097  -> 145529
   * 96687  -> 207119
   * 154308 -> 29540 (w)
   * 202668 -> 77900 (w)
   * 18517  -> 128949
   * 4945   -> 115377
   */
  for (int i = 0; i < sizeof(start_times) / sizeof(uint32_t); i++) {
    init_audio_sample(&audio_samples[i], sizeof(int16_t) * length, val, name,
                      start_times[i], end);
    audio_samples[i].next = i != sizeof(start_times) / sizeof(uint32_t) - 1
                                ? &audio_samples[i + 1]
                                : NULL;
  }

  const int buf_size = 58800;
  int16_t *buf = calloc(1, buf_size);
  const uint32_t half_buf_size = buf_size / 2;

  uint32_t start_gap = 0;
  uint32_t end_gap = (start_gap + half_buf_size / 2) % end;
  int16_t *buf_start = buf;
  int16_t *buf_end = buf_start + half_buf_size / 2;
  fill_buffer(audio_samples, buf_start, buf_end, start_gap, end_gap, end);
  start_gap = end_gap;

  end_gap = (start_gap + half_buf_size / 2) % end;
  buf_start = buf_end;
  buf_end = buf_start + half_buf_size / 2;
  fill_buffer(audio_samples, buf_start, buf_end, start_gap, end_gap, end);
  start_gap = end_gap;

  // 0 - 29400
  for (int i = 0; i < 4945; i++) {
    int16_t res_val = 2 * (0x11 << 8 | 0x11);
    TEST_ASSERT_EQUAL(res_val, buf[i]);
  }
  for (int i = 4945; i < 18517; i++) {
    int16_t res_val = 3 * (0x11 << 8 | 0x11);
    TEST_ASSERT_EQUAL(res_val, buf[i]);
  }
  for (int i = 18517; i < 29400; i++) {
    int16_t res_val = 4 * (0x11 << 8 | 0x11);
    TEST_ASSERT_EQUAL(res_val, buf[i]);
  }
  memset(buf, 0, buf_size);
  end_gap = (start_gap + half_buf_size / 2) % end;
  buf_start = buf;
  buf_end = buf_start + half_buf_size / 2;
  fill_buffer(audio_samples, buf_start, buf_end, start_gap, end_gap, end);
  start_gap = end_gap;

  end_gap = (start_gap + half_buf_size / 2) % end;
  buf_start = buf_end;
  buf_end = buf_start + half_buf_size / 2;
  fill_buffer(audio_samples, buf_start, buf_end, start_gap, end_gap, end);
  start_gap = end_gap;

  uint32_t diff = 29400;
  // 29400 - 58800
  for (int i = 29400; i < 29540; i++) {
    int16_t res_val = 4 * (0x11 << 8 | 0x11);
    TEST_ASSERT_EQUAL(res_val, buf[i - diff]);
  }

  for (int i = 29540; i < 35097; i++) {
    int16_t res_val = 3 * (0x11 << 8 | 0x11);
    TEST_ASSERT_EQUAL(res_val, buf[i - diff]);
  }

  for (int i = 35097; i < 58800; i++) {
    int16_t res_val = 4 * (0x11 << 8 | 0x11);
    TEST_ASSERT_EQUAL(res_val, buf[i - diff]);
  }

  memset(buf, 0, buf_size);
  end_gap = (start_gap + half_buf_size / 2) % end;
  buf_start = buf;
  buf_end = buf_start + half_buf_size / 2;
  fill_buffer(audio_samples, buf_start, buf_end, start_gap, end_gap, end);
  start_gap = end_gap;

  end_gap = (start_gap + half_buf_size / 2) % end;
  buf_start = buf_end;
  buf_end = buf_start + half_buf_size / 2;
  fill_buffer(audio_samples, buf_start, buf_end, start_gap, end_gap, end);
  start_gap = end_gap;

  diff = 58800;
  // 58800 - 88200
  for (int i = 58800; i < 77900; i++) {
    int16_t res_val = 4 * (0x11 << 8 | 0x11);
    TEST_ASSERT_EQUAL(res_val, buf[i - diff]);
  }

  for (int i = 77900; i < 88200; i++) {
    int16_t res_val = 3 * (0x11 << 8 | 0x11);
    TEST_ASSERT_EQUAL(res_val, buf[i - diff]);
  }

  memset(buf, 0, buf_size);
  end_gap = (start_gap + half_buf_size / 2) % end;
  buf_start = buf;
  buf_end = buf_start + half_buf_size / 2;
  fill_buffer(audio_samples, buf_start, buf_end, start_gap, end_gap, end);
  start_gap = end_gap;

  end_gap = (start_gap + half_buf_size / 2) % end;
  buf_start = buf_end;
  buf_end = buf_start + half_buf_size / 2;
  fill_buffer(audio_samples, buf_start, buf_end, start_gap, end_gap, end);
  start_gap = end_gap;

  diff = 88200;
  // 88200 - 117600
  for (int i = 88200; i < 96687; i++) {
    int16_t res_val = 3 * (0x11 << 8 | 0x11);
    TEST_ASSERT_EQUAL(res_val, buf[i - diff]);
  }

  for (int i = 96687; i < 115377; i++) {
    int16_t res_val = 4 * (0x11 << 8 | 0x11);
    TEST_ASSERT_EQUAL(res_val, buf[i - diff]);
  }

  for (int i = 115377; i < 117600; i++) {
    int16_t res_val = 3 * (0x11 << 8 | 0x11);
    TEST_ASSERT_EQUAL(res_val, buf[i - diff]);
  }

  memset(buf, 0, buf_size);
  end_gap = (start_gap + half_buf_size / 2) % end;
  buf_start = buf;
  buf_end = buf_start + half_buf_size / 2;
  fill_buffer(audio_samples, buf_start, buf_end, start_gap, end_gap, end);
  start_gap = end_gap;

  end_gap = (start_gap + half_buf_size / 2) % end;
  buf_start = buf_end;
  buf_end = buf_start + half_buf_size / 2;
  fill_buffer(audio_samples, buf_start, buf_end, start_gap, end_gap, end);
  start_gap = end_gap;

  diff = 117600;
  // 117600 - 147000
  for (int i = 117600; i < 128949; i++) {
    int16_t res_val = 3 * (0x11 << 8 | 0x11);
    TEST_ASSERT_EQUAL(res_val, buf[i - diff]);
  }

  for (int i = 128949; i < 145529; i++) {
    int16_t res_val = 2 * (0x11 << 8 | 0x11);
    TEST_ASSERT_EQUAL(res_val, buf[i - diff]);
  }

  for (int i = 145529; i < 147000; i++) {
    int16_t res_val = 1 * (0x11 << 8 | 0x11);
    TEST_ASSERT_EQUAL(res_val, buf[i - diff]);
  }

  memset(buf, 0, buf_size);
  end_gap = (start_gap + half_buf_size / 2) % end;
  buf_start = buf;
  buf_end = buf_start + half_buf_size / 2;
  fill_buffer(audio_samples, buf_start, buf_end, start_gap, end_gap, end);
  start_gap = end_gap;

  end_gap = (start_gap + half_buf_size / 2) % end;
  buf_start = buf_end;
  buf_end = buf_start + half_buf_size / 2;
  fill_buffer(audio_samples, buf_start, buf_end, start_gap, end_gap, end);
  start_gap = end_gap;

  diff = 147000;
  // 147000 - 176400
  for (int i = 147000; i < 154308; i++) {
    int16_t res_val = 1 * (0x11 << 8 | 0x11);
    TEST_ASSERT_EQUAL(res_val, buf[i - diff]);
  }

  for (int i = 154308; i < 176400; i++) {
    int16_t res_val = 2 * (0x11 << 8 | 0x11);
    TEST_ASSERT_EQUAL(res_val, buf[i - diff]);
  }

  memset(buf, 0, buf_size);
  end_gap = (start_gap + half_buf_size / 2) % end;
  buf_start = buf;
  buf_end = buf_start + half_buf_size / 2;
  fill_buffer(audio_samples, buf_start, buf_end, start_gap, end_gap, end);
  start_gap = end_gap;

  end_gap = (start_gap + half_buf_size / 2) % end;
  buf_start = buf_end;
  buf_end = buf_start + half_buf_size / 2;
  fill_buffer(audio_samples, buf_start, buf_end, start_gap, end_gap, end);
  start_gap = end_gap;

  diff = 176400;
  // 176400 - 205800
  for (int i = 176400; i < 202668; i++) {
    int16_t res_val = 2 * (0x11 << 8 | 0x11);
    TEST_ASSERT_EQUAL(res_val, buf[i - diff]);
  }

  for (int i = 202668; i < 205800; i++) {
    int16_t res_val = 3 * (0x11 << 8 | 0x11);
    TEST_ASSERT_EQUAL(res_val, buf[i - diff]);
  }

  memset(buf, 0, buf_size);
  end_gap = (start_gap + half_buf_size / 2) % end;
  buf_start = buf;
  buf_end = buf_start + half_buf_size / 2;
  fill_buffer(audio_samples, buf_start, buf_end, start_gap, end_gap, end);
  start_gap = end_gap;

  end_gap = (start_gap + half_buf_size / 2) % end;
  buf_start = buf_end;
  buf_end = buf_start + half_buf_size / 2;
  fill_buffer(audio_samples, buf_start, buf_end, start_gap, end_gap, end);
  start_gap = end_gap;

  diff = 205800;
  // 205800 - 235200
  for (int i = 205800; i < 207119; i++) {
    int16_t res_val = 3 * (0x11 << 8 | 0x11);
    TEST_ASSERT_EQUAL(res_val, buf[i - diff]);
  }

  for (int i = 207119; i < 235200; i++) {
    int16_t res_val = 2 * (0x11 << 8 | 0x11);
    TEST_ASSERT_EQUAL(res_val, buf[i - diff]);
  }

  memset(buf, 0, buf_size);
  end_gap = (start_gap + half_buf_size / 2) % end;
  buf_start = buf;
  buf_end = buf_start + half_buf_size / 2;
  fill_buffer(audio_samples, buf_start, buf_end, start_gap, end_gap, end);
  start_gap = end_gap;

  end_gap = (start_gap + half_buf_size / 2) % end;
  buf_start = buf_end;
  buf_end = buf_start + half_buf_size / 2;
  fill_buffer(audio_samples, buf_start, buf_end, start_gap, end_gap, end);
  start_gap = end_gap;

  for (int i = 0; i < 4945; i++) {
    int16_t res_val = 2 * (0x11 << 8 | 0x11);
    TEST_ASSERT_EQUAL(res_val, buf[i]);
  }
  for (int i = 4945; i < 18517; i++) {
    int16_t res_val = 3 * (0x11 << 8 | 0x11);
    TEST_ASSERT_EQUAL(res_val, buf[i]);
  }
  for (int i = 18517; i < 29400; i++) {
    int16_t res_val = 4 * (0x11 << 8 | 0x11);
    TEST_ASSERT_EQUAL(res_val, buf[i]);
  }

  for (int i = 0; i < sizeof(start_times) / sizeof(uint32_t); i++) {
    deinit_audio_sample(&audio_samples[i]);
  }
}

void test_fill_single_transcending_sample(void) {
  struct audio_sample audio_sample = {0};

  const uint32_t end = 1024;
  init_audio_sample(&audio_sample, 256, 0x44, "A.wav", 1024 - 64, end);

  const int buf_size = 1024;
  int16_t *buf = calloc(1, buf_size);
  int16_t value = 0;

  fill_buffer(&audio_sample, buf, buf + (buf_size / sizeof(uint16_t)),
              1024 - 64 - 2, 64 + 2 + 2, end);

  for (int i = 0; i < 4 / sizeof(uint16_t); i++) {
    TEST_ASSERT_EQUAL(0, buf[i]);
  }
  value = mix_val(0, 0x44);
  for (int i = 4 / sizeof(int16_t); i < 260 / sizeof(int16_t); i++) {
    TEST_ASSERT_EQUAL(value, buf[i]);
  }
  for (int i = 260 / sizeof(int16_t); i < 1024 / sizeof(int16_t); i++) {
    TEST_ASSERT_EQUAL(0, buf[i]);
  }

  memset(buf, 0, buf_size);

  fill_buffer(&audio_sample, buf, buf + (buf_size / sizeof(uint16_t)),
              1024 - 64, 64 + 2, end);

  for (int i = 0 / sizeof(int16_t); i < 256 / sizeof(int16_t); i++) {
    TEST_ASSERT_EQUAL(value, buf[i]);
  }
  for (int i = 256 / sizeof(int16_t); i < 1024 / sizeof(int16_t); i++) {
    TEST_ASSERT_EQUAL(0, buf[i]);
  }

  memset(buf, 0, buf_size);

  fill_buffer(&audio_sample, buf, buf + (buf_size / sizeof(int16_t)), 1024 - 32,
              32, end);
  for (int i = 0; i < 128 / sizeof(int16_t); i++) {
    char msg[64] = {0};
    sprintf(msg, "%d", i);
    TEST_ASSERT_EQUAL_MESSAGE(value, buf[i], msg);
  }
  for (int i = 128 / sizeof(int16_t); i < 1024 / sizeof(int16_t); i++) {
    TEST_ASSERT_EQUAL(0, buf[i]);
  }

  memset(buf, 0, buf_size);

  fill_buffer(&audio_sample, buf, buf + (buf_size / sizeof(int16_t)), 1024 - 32,
              1024 - 16, end);
  for (int i = 0; i < 32 / sizeof(int16_t); i++) {
    TEST_ASSERT_EQUAL(value, buf[i]);
  }
  for (int i = 32 / sizeof(int16_t); i < 1024 / sizeof(int16_t); i++) {
    TEST_ASSERT_EQUAL(0, buf[i]);
  }

  memset(buf, 0, buf_size);

  fill_buffer(&audio_sample, buf, buf + (buf_size / sizeof(int16_t)), 4, 8,
              end);
  for (int i = 0; i < 8 / sizeof(int16_t); i++) {
    TEST_ASSERT_EQUAL(value, buf[i]);
  }
  for (int i = 8 / sizeof(int16_t); i < 1024 / sizeof(int16_t); i++) {
    TEST_ASSERT_EQUAL(0, buf[i]);
  }

  memset(buf, 0, buf_size);

  fill_buffer(&audio_sample, buf, buf + (buf_size / sizeof(int16_t)), 4, 128,
              end);
  for (int i = 0; i < 120 / sizeof(int16_t); i++) {
    TEST_ASSERT_EQUAL(value, buf[i]);
  }
  for (int i = 120 / sizeof(int16_t); i < 1024 / sizeof(int16_t); i++) {
    TEST_ASSERT_EQUAL(0, buf[i]);
  }

  memset(buf, 0, buf_size);

  fill_buffer(&audio_sample, buf, buf + (buf_size / sizeof(int16_t)),
              1024 - 64 - 8, 32, end);
  for (int i = 0; i < 16 / sizeof(int16_t); i++) {
    TEST_ASSERT_EQUAL(0, buf[i]);
  }
  for (int i = 16 / sizeof(int16_t); i < (128 + 64 + 16) / sizeof(int16_t);
       i++) {
    TEST_ASSERT_EQUAL(value, buf[i]);
  }
  for (int i = (128 + 64 + 16) / sizeof(int16_t); i < 1024 / sizeof(int16_t);
       i++) {
    TEST_ASSERT_EQUAL(0, buf[i]);
  }

  memset(buf, 0, buf_size);

  fill_buffer(&audio_sample, buf, buf + (buf_size / sizeof(int16_t)),
              1024 - 64 - 8, 68, end);
  for (int i = 0; i < 16 / sizeof(int16_t); i++) {
    TEST_ASSERT_EQUAL(0, buf[i]);
  }
  for (int i = 16 / sizeof(int16_t); i < (128 + 128 + 16) / sizeof(int16_t);
       i++) {
    TEST_ASSERT_EQUAL(value, buf[i]);
  }
  for (int i = (128 + 128 + 16) / sizeof(int16_t); i < 1024 / sizeof(int16_t);
       i++) {
    TEST_ASSERT_EQUAL(0, buf[i]);
  }

  memset(buf, 0, buf_size);

  fill_buffer(&audio_sample, buf, buf + (buf_size / sizeof(int16_t)),
              1024 - 64 + 8, 32, end);
  for (int i = 0; i < (128 + 64 - 16) / sizeof(int16_t); i++) {
    TEST_ASSERT_EQUAL(value, buf[i]);
  }
  for (int i = (128 + 64 - 16) / sizeof(int16_t); i < 1024 / sizeof(int16_t);
       i++) {
    TEST_ASSERT_EQUAL(0, buf[i]);
  }

  memset(buf, 0, buf_size);

  fill_buffer(&audio_sample, buf, buf + (buf_size / sizeof(int16_t)),
              1024 - 64 + 8, 68, end);
  for (int i = 0; i < (128 + 128 - 16) / sizeof(int16_t); i++) {
    TEST_ASSERT_EQUAL(value, buf[i]);
  }
  for (int i = (128 + 128 - 16) / sizeof(int16_t); i < 1024 / sizeof(int16_t);
       i++) {
    TEST_ASSERT_EQUAL(0, buf[i]);
  }

  deinit_audio_sample(&audio_sample);
  free(buf);
}

int main(void) {
  UNITY_BEGIN();
  RUN_TEST(test_fill_simple_case);
  RUN_TEST(test_fill_multiple_samples_simple_case);
  RUN_TEST(test_fill_single_transcending_sample);
  RUN_TEST(test_problem);
  return UNITY_END();
}
