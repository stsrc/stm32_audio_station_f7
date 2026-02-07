#include "buffer_fill.h"

static void copy_bytes(int16_t *sample_ptr, int16_t *buf_ptr,
                       int16_t *sample_stop, int16_t *buf_stop_ptr) {
  while (sample_ptr < sample_stop && buf_ptr < buf_stop_ptr) {
    int32_t tmp = *sample_ptr + *buf_ptr;
    if (tmp != (int16_t)tmp) {
      if (tmp > INT16_MAX) {
        tmp = INT16_MAX;
      } else {
        tmp = INT16_MIN;
      }
    }
    *buf_ptr = (int16_t)tmp;
    buf_ptr++;
    sample_ptr++;
  }
  if (sample_ptr != sample_stop || buf_ptr != buf_stop_ptr) {
    while (1)
      ;
  }
}

void fill_buffer_with_sample(struct audio_sample *add, int16_t *buf_start,
                             int16_t *buf_end, const uint32_t start_gap,
                             const uint32_t end_gap, const uint32_t end) {
  if (start_gap >= end) {
    while (1)
      ;
  }

  int16_t *sample_start, *sample_stop, *buf_start_ptr, *buf_stop_ptr,
      *sample_ptr, *buf_ptr;

  if (add->start_sample < add->stop_sample) {
    if ((start_gap < end_gap &&
         (start_gap > add->stop_sample || end_gap < add->start_sample)) ||
        ((start_gap > end_gap) &&
         (end_gap < add->start_sample && start_gap < add->stop_sample))) {
      return;
    }
    if (start_gap < end_gap) {
      if (add->start_sample < start_gap) {
        sample_start = add->sample->buf + (start_gap - add->start_sample);
        buf_start_ptr = buf_start;
      } else {
        sample_start = add->sample->buf;
        buf_start_ptr = buf_start + (add->start_sample - start_gap);
      }
      if (add->stop_sample < end_gap) {
        sample_stop = add->sample->buf + (add->stop_sample - add->start_sample);
        buf_stop_ptr = buf_start + (add->stop_sample - start_gap);
      } else {
        sample_stop = add->sample->buf + (end_gap - add->start_sample);
        buf_stop_ptr = buf_start + (end_gap - start_gap);
      }
      sample_ptr = sample_start;
      buf_ptr = buf_start_ptr;
      copy_bytes(sample_ptr, buf_ptr, sample_stop, buf_stop_ptr);
    } else {
      if (add->start_sample > start_gap) {
        sample_start = add->sample->buf;
        buf_start_ptr = buf_start + (add->start_sample - start_gap);
        if (add->stop_sample < end) {
          sample_stop =
              add->sample->buf + (add->stop_sample - add->start_sample);
          buf_stop_ptr = buf_start + (add->stop_sample - add->start_sample);
        } else {
          sample_stop = add->sample->buf + (end - add->start_sample);
          buf_stop_ptr = buf_start + (end - start_gap);
        }
        sample_ptr = sample_start;
        buf_ptr = buf_start_ptr;

        copy_bytes(sample_ptr, buf_ptr, sample_stop, buf_stop_ptr);

        sample_start = sample_stop;
        buf_start_ptr = buf_stop_ptr;
        sample_ptr = sample_start;
      } else {
        sample_start = add->sample->buf;
        buf_start_ptr = buf_start + (end - start_gap + add->start_sample);
        sample_ptr = sample_start;
      }
      if (add->stop_sample < end_gap) {
        sample_stop = sample_start + add->stop_sample;
        buf_stop_ptr = buf_start_ptr + add->stop_sample;
      } else {
        sample_stop = sample_start + end_gap;
        buf_stop_ptr = buf_start_ptr + end_gap;
      }

      sample_ptr = sample_start;
      buf_ptr = buf_start_ptr;
      copy_bytes(sample_ptr, buf_ptr, sample_stop, buf_stop_ptr);
    }
  } else {
    if (start_gap < end_gap && start_gap < add->start_sample &&
        end_gap > add->stop_sample && start_gap > add->stop_sample &&
        end_gap < add->start_sample) {
      return;
    }
    if (start_gap < end_gap) {
      if (start_gap < add->start_sample && start_gap < add->stop_sample) {
        sample_start = add->sample->buf + (end - add->start_sample) + start_gap;
        buf_start_ptr = buf_start;
        if (add->stop_sample < end_gap) {
          sample_stop =
              add->sample->buf + (end - add->start_sample) + add->stop_sample;
          buf_stop_ptr = buf_start_ptr + (add->stop_sample - start_gap);
        } else {
          sample_stop = add->sample->buf + (end - add->start_sample) + end_gap;
          buf_stop_ptr = buf_start_ptr + end_gap - start_gap;
        }
        sample_ptr = sample_start;
        buf_ptr = buf_start_ptr;
        copy_bytes(sample_ptr, buf_ptr, sample_stop, buf_stop_ptr);
      } else if (start_gap < add->start_sample &&
                 start_gap > add->stop_sample) {
        sample_start = add->sample->buf;
        buf_start_ptr = buf_start + (add->start_sample - start_gap);
        sample_stop = add->sample->buf + (end_gap - add->start_sample);
        buf_stop_ptr = buf_start_ptr + (end_gap - add->start_sample);
        sample_ptr = sample_start;
        buf_ptr = buf_start_ptr;
        copy_bytes(sample_ptr, buf_ptr, sample_stop, buf_stop_ptr);
      } else { // start_gap > add->start_sample && end_gap < end
        sample_start = add->sample->buf + (start_gap - add->start_sample);
        buf_start_ptr = buf_start;
        sample_stop = add->sample->buf + (start_gap - add->start_sample) +
                      (end_gap - start_gap);
        buf_stop_ptr = buf_start + (end_gap - start_gap);
        sample_ptr = sample_start;
        buf_ptr = buf_start_ptr;
        copy_bytes(sample_ptr, buf_ptr, sample_stop, buf_stop_ptr);
      }
    } else {
      if (start_gap < add->start_sample) {
        sample_start = add->sample->buf;
        buf_start_ptr = buf_start + (add->start_sample - start_gap);
        sample_stop = add->sample->buf + (end - add->start_sample);
        buf_stop_ptr = buf_start + (end - start_gap);
      } else {
        sample_start = add->sample->buf + (start_gap - add->start_sample);
        buf_start_ptr = buf_start;
        sample_stop = add->sample->buf + (end - add->start_sample);
        buf_stop_ptr = buf_start + (end - start_gap);
      }
      sample_ptr = sample_start;
      buf_ptr = buf_start_ptr;
      copy_bytes(sample_ptr, buf_ptr, sample_stop, buf_stop_ptr);
      sample_start = sample_stop;
      buf_start_ptr = buf_stop_ptr;
      if (end_gap < add->stop_sample) {
        sample_stop = sample_start + end_gap;
        buf_stop_ptr = buf_start_ptr + end_gap;
      } else {
        sample_stop = sample_start + add->stop_sample;
        buf_stop_ptr = buf_start_ptr + add->stop_sample;
      }
      sample_ptr = sample_start;
      buf_ptr = buf_start_ptr;
      copy_bytes(sample_ptr, buf_ptr, sample_stop, buf_stop_ptr);
    }
  }
}

void fill_buffer(struct audio_sample *head, int16_t *buf_start,
                 int16_t *buf_end, const uint32_t start_gap,
                 const uint32_t end_gap, const uint32_t end) {
  struct audio_sample *add = head;

  while (add && add->sample) {
    fill_buffer_with_sample(add, buf_start, buf_end, start_gap, end_gap, end);
    add = add->next;
  }
}
