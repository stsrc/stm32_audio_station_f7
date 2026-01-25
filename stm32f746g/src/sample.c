#include "sample.h"
#include "sd_diskio.h"

#include <cmsis_os.h>
#include <stdio.h>
#include <string.h>

struct wavheader {
  /* RIFF Chunk Descriptor */
  uint8_t RIFF[4];    // RIFF Header Magic header
  uint32_t ChunkSize; // RIFF Chunk Size
  uint8_t WAVE[4];    // WAVE Header
  /* "fmt" sub-chunk */
  uint8_t fmt[4];         // FMT header
  uint32_t Subchunk1Size; // Size of the fmt chunk
  uint16_t AudioFormat;   // Audio format 1=PCM,6=mulaw,7=alaw,     257=IBM
                          // Mu-Law, 258=IBM A-Law, 259=ADPCM
  uint16_t NumOfChan;     // Number of channels 1=Mono 2=Sterio
  uint32_t SamplesPerSec; // Sampling Frequency in Hz
  uint32_t bytesPerSec;   // bytes per second
  uint16_t blockAlign;    // 2=16-bit mono, 4=16-bit stereo
  uint16_t bitsPerSample; // Number of bits per sample
  /* "data" sub-chunk */
  uint8_t Subchunk2ID[4]; // "data"  string
  uint32_t Subchunk2Size; // Sampled data length
};

static bool initialized = false;
static FATFS sd_fat_fs = {0};
static char sd_path[16] = {0};

struct sample_queue {
	struct sample *sample;
	struct sample_queue *next;
};

static struct sample_queue *head = NULL;

bool sample_init() {
  if (initialized)
    return true;

  if (FATFS_LinkDriver(&sd_op, sd_path)) {
    printf("Failed to initialize fatfs\n");
    return false;
  }

  if (f_mount(&sd_fat_fs, (TCHAR const *)sd_path, 0)) {
    printf("Failed to mount partition\n");
    return false;
  }

  head = pvPortMalloc(sizeof(struct sample_queue));
  if (!head) {
	  printf("No memory\n");
	  return false;
  }
  memset(head, 0, sizeof(struct sample_queue));
  initialized = true;
  return true;
}

static bool __sample_open(struct sample *sample, const char *name) {
  if (!initialized) {
    return false;
  }

  FIL fil = {0};

  // +1: '\0'
  size_t size = strlen(sd_path) + strlen(name) + 1;
  char filename[size];
  memset(filename, 0, size);
  strcpy(filename, sd_path);
  strcat(filename, name);
  FRESULT fr = f_open(&fil, filename, FA_READ);
  if (fr != FR_OK) {
    f_close(&fil);
    printf("Can't open file\n");
    return false;
  }

  struct wavheader wavheader = {0};

  DWORD file_size = f_size(&fil);
  if (file_size <= sizeof(struct wavheader)) {
    f_close(&fil);
    printf("File not wav?\n");
    return false;
  }

  UINT br;
  fr = f_read(&fil, (void *)&wavheader, sizeof(struct wavheader), &br);
  if (br != sizeof(struct wavheader) || fr != FR_OK) {
    f_close(&fil);
    printf("Could not read wavheader\n");
    return false;
  } else if (strncmp((const char *)wavheader.RIFF, "RIFF", 4) ||
             strncmp((const char *)wavheader.WAVE, "WAVE", 4)) {
    f_close(&fil);
    printf("Headers magic incorrect\n");
    return false;
  }

  const uint32_t wav_size = file_size - sizeof(struct wavheader);
  int16_t *buf = pvPortMalloc(wav_size);
  if (!buf) {
    f_close(&fil);
    printf("No memory for file content\n");
    return false;
  }

  fr = f_read(&fil, (void *)buf, wav_size, &br);
  f_close(&fil);
  if (wav_size != br || fr != FR_OK) {
    printf("Could not read file content\n");
    return false;
  }
  sample->buf = buf;
  sample->bytes_size = wav_size;
  sample->samples_per_sec = wavheader.SamplesPerSec;
  sample->name = strdup(name);

  return true;
}

static void __sample_delete(struct sample *sample) {
  vPortFree(sample->buf);
  vPortFree(sample->name);
  memset(sample, 0, sizeof(struct sample));
}

bool sample_open(const char *name) {
	struct sample_queue *tmp;
	for (tmp = head; tmp->sample != NULL; tmp = tmp->next) {
                if (!strcmp(tmp->sample->name, name)) {
			return true;
		}
	}

	struct sample_queue *queue_el = pvPortMalloc(sizeof(struct sample_queue));
	if (!queue_el) {
		return false;
	}
	memset(queue_el, 0, sizeof(struct sample_queue));
	struct sample *ptr = pvPortMalloc(sizeof(struct sample));
	if (!ptr) {
		vPortFree(ptr);
		return false;
	}
	memset(ptr, 0, sizeof(struct sample));

	if (!__sample_open(ptr, name)) {
		vPortFree(ptr);
		vPortFree(queue_el);
		return false;
	}
	tmp->next = queue_el;
	tmp->sample = ptr;

	return true;
}

struct sample* sample_get(const char *name) {
	for (struct sample_queue *tmp = head; tmp->sample != NULL; tmp = tmp->next) {
		if (!strcmp(tmp->sample->name, name)) {
			return tmp->sample;
		}
	}
	return NULL;
}

void sample_delete(const char *name) {
	struct sample_queue *prev = head;
	for (struct sample_queue *tmp = prev->next; tmp->sample != NULL; tmp = tmp->next) {
		if (!strcmp(tmp->sample->name, name)) {
			if (prev) {
			  prev->next = tmp->next;
			}
			__sample_delete(tmp->sample);
			vPortFree(tmp);
		} else {
			prev = tmp;
		}
	}
}
