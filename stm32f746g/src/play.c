#include "play.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <stm32746g_discovery_audio.h>

#include "buffer_fill.h"
#include "sample.h"

osMessageQId Play_MessageId;

static __IO struct play_message irq_msg = {.op = FILL_BUFFER};

// Think what should have __IO!

static __IO uint32_t buffer_start = 0;
static __IO uint32_t buffer_stop = 0;
uint32_t buf_size = 0;
static uint32_t end = 0;

void BSP_AUDIO_OUT_TransferComplete_CallBack(void) {
  // Now following section will be played
  buffer_start = buffer_stop;
  buffer_stop = (buffer_stop + buf_size / 2) % end;

  irq_msg.data.first_half = false;
  osMessagePut(Play_MessageId, (uint32_t)&irq_msg, 0);
}

void BSP_AUDIO_OUT_HalfTransfer_CallBack(void) {
  irq_msg.data.first_half = true;
  osMessagePut(Play_MessageId, (uint32_t)&irq_msg, 0);
}

#define AUDIO_BUF_SIZ 0x10000
static char *audio_buffer = NULL;

float half_bar_length_s = 0.0f;
float four_bars_length_s = 0.0f;
uint32_t to_fill_start = 0;
static struct audio_sample head = {0};

struct play play_settings = {0};

// sample rate means 16-bit pairs for left and right channel, so 2 times more
static uint32_t time_to_samples(float time, uint32_t samples_per_sec) {
  return 2 * (uint32_t)(time * (float)samples_per_sec + 0.5f);
}

void fill_half_buffer(bool first_half) {
  const uint32_t half_buf_size = buf_size / 2;
  int16_t *buf_start, *buf_end;
  if (first_half) {
    buf_start = (int16_t *)audio_buffer;
  } else {
    buf_start = ((int16_t *)audio_buffer) + half_buf_size / 2;
  }
  memset(buf_start, 0, half_buf_size);
  buf_end = buf_start + half_buf_size / 2;

  const uint32_t start_gap = to_fill_start;
  uint32_t end_gap = start_gap + half_buf_size / 2;
  if (end_gap > end) {
	end_gap %= end;
  }
  fill_buffer(&head, buf_start, buf_end, start_gap, end_gap, end);
  to_fill_start = end_gap % end;
}

static struct audio_sample *play_add_sample(struct sample *sample,
                                            uint32_t start_time) {
  if (start_time >= end) {
    printf("Wrong start time of sample (%lu >= %lu)\n", start_time, end);
    return NULL;
  }

  struct audio_sample *add = &head;
  while (add->sample && add->next) {
    add = add->next;
  }
  add->sample = sample;
  add->start_sample = start_time;
  add->start_sample += add->start_sample % 2;
  add->stop_sample = add->start_sample + add->sample->bytes_size / 2;
  if (add->stop_sample > end) {
    add->stop_sample -= end;
    if (add->stop_sample > add->start_sample) {
      add->stop_sample = add->start_sample;
    }
  }
  add->next = calloc(1, sizeof(struct audio_sample));
  return add;
}

int play_add_sample_at_sec(struct sample *sample, float start_time) {
  if (start_time > four_bars_length_s) {
    printf("Start time exceeds 4 bar time\n");
    return -1;
  }

  return play_add_sample(
             sample, time_to_samples(start_time, sample->samples_per_sec)) ==
         NULL;
}

static void _remove_at_head() {
  struct audio_sample *tmp = head.next;
  head = *head.next;
  free(tmp);
}

void play_remove_all() {
  while (head.sample) {
    _remove_at_head();
  }
}

static void play_remove_all_matching_samples(const char *name) {
  struct audio_sample *remove = &head;
  struct audio_sample *prev = NULL;
  while (remove->sample) {
    if (!strcmp(remove->sample->name, name)) {
      if (prev) {
        prev->next = remove->next;
        free(remove);
        remove = prev->next;
      } else {
        _remove_at_head();
      }
    } else {
      prev = remove;
      remove = remove->next;
    }
  }
}

static void play_setup() {
  if (play_settings.running) {
	return;
  }
  half_bar_length_s = 0.5f / (((float)play_settings.BPM) / 60.0f);
  four_bars_length_s = 2.0f * half_bar_length_s * 4.0f;

  end = time_to_samples(four_bars_length_s, play_settings.samples_per_sec);
  end += end % 2;

  buf_size = (half_bar_length_s * (float)play_settings.samples_per_sec) * 2.0f + 0.5f;
  buf_size += buf_size % 2;
  buf_size *= sizeof(uint16_t);
  if (buf_size > AUDIO_BUF_SIZ) {
    buf_size = AUDIO_BUF_SIZ;
  }

  audio_buffer = pvPortMalloc(buf_size);
  memset(audio_buffer, 0, buf_size);

  if (BSP_AUDIO_OUT_Init(OUTPUT_DEVICE_BOTH, 50, play_settings.samples_per_sec)) {
    while (1)
      ;
  }
  BSP_AUDIO_OUT_SetAudioFrameSlot(CODEC_AUDIOFRAME_SLOT_02);

  buffer_start = 0;
  buffer_stop = (buf_size / 2) % end;
  to_fill_start = 0;
  for (int i = 0; i < 2; i++) {
    fill_half_buffer(!i);
  }
  BSP_AUDIO_OUT_Play((uint16_t *)audio_buffer, buf_size);
  play_settings.running = true;
}

extern SAI_HandleTypeDef haudio_out_sai;

#define MARGIN 32

static void play_add_sample_now(const char *name) {
  if (!play_settings.running) {
    return;
  }

  uint32_t ndtr = haudio_out_sai.hdmatx->Instance->NDTR;
  uint32_t samples_sent = (buf_size / 2 - ndtr + MARGIN) % (buf_size / 2);
  uint32_t b_start = buffer_start;
  uint32_t sample_start = (b_start + samples_sent) % end;
  struct sample *sample = sample_get(name);
  if (!sample) {
    return;
  }

  struct audio_sample *add = play_add_sample(sample, sample_start);
  if (samples_sent < buf_size / 2 / 2) {
    uint32_t start_gap = b_start;
    uint32_t middle_gap = (start_gap + buf_size / 4) % end;
    uint32_t end_gap = (middle_gap + buf_size / 4) % end;
    fill_buffer_with_sample(add, (int16_t *)audio_buffer,
                            ((int16_t *)audio_buffer) + buf_size / 4, start_gap,
                            middle_gap, end);
    fill_buffer_with_sample(add, ((int16_t *)audio_buffer) + buf_size / 4,
                            ((int16_t *)audio_buffer) + buf_size / 2,
                            middle_gap, end_gap, end);
  } else {
    uint32_t start_gap = (b_start + buf_size / 4) % end;
    uint32_t middle_gap = (start_gap + buf_size / 4) % end;
    uint32_t end_gap = (middle_gap + buf_size / 4) % end;
    fill_buffer_with_sample(add, ((int16_t *)audio_buffer) + buf_size / 4,
                            ((int16_t *)audio_buffer) + buf_size / 2, start_gap,
                            middle_gap, end);
    fill_buffer_with_sample(add, (int16_t *)audio_buffer,
                            ((int16_t *)audio_buffer) + buf_size / 4,
                            middle_gap, end_gap, end);
  }
}

static void __enable_metronome() {
  struct sample *sample = sample_get(play_settings.metronome_name);
  for (uint32_t i = 0; i < end; i += end / 4) {
    play_add_sample(sample, i);
  }
}
static void play_handle_metronome() {
  if (!play_settings.metronome_name) {
    printf("Metronome name sample not set\n");
    return;
  }
  if (!play_settings.metronome_enabled) {
    __enable_metronome();
  } else {
    play_remove_all_matching_samples(play_settings.metronome_name);
  }
  play_settings.metronome_enabled = !play_settings.metronome_enabled;
}

void play_thread(void const *arg) {
  while (1) {
    osEvent evt = osMessageGet(Play_MessageId, osWaitForever);
    struct play_message *msg = (struct play_message *)evt.value.p;
    switch (msg->op) {
    case NA:
      free(msg);
      break;
    case SETUP:
      play_settings.BPM = msg->data.settings.BPM;
      play_settings.samples_per_sec = msg->data.settings.samples_per_sec;
      play_setup();
      free(msg);
      break;
    case RESUME:
      BSP_AUDIO_OUT_Resume();
      free(msg);
      break;
    case PAUSE:
      BSP_AUDIO_OUT_Pause();
      free(msg);
      break;
    case STOP:
      if (play_settings.running) {
//        BSP_AUDIO_OUT_Stop(CODEC_PDWN_SW);
        BSP_AUDIO_OUT_DeInit();
        vPortFree(audio_buffer);
        play_settings.running = false;
      }
      free(msg);
      break;
    case DELETE_ALL:
      play_remove_all();
      play_settings.metronome_enabled = false;
      free(msg);
      break;
    case ADD:
      play_add_sample_now(msg->data.name);
      free(msg);
      break;
    case FILL_BUFFER:
      fill_half_buffer(msg->data.first_half);
      break;
    case METRONOME:
      if (play_settings.metronome_name) {
        if (strcmp(play_settings.metronome_name, msg->data.name)) {
          free((void *)play_settings.metronome_name);
          play_settings.metronome_name = strdup(msg->data.name);
        }
      } else {
        play_settings.metronome_name = strdup(msg->data.name);
      }
      play_handle_metronome();
      free(msg);
      break;
    default:
    }
  }
}
