#include "synth.h"
#define _USE_MATH_DEFINES
#include <math.h>

#ifdef STM32F746xx
#include <cmsis_os.h>
#else
#include <stdlib.h>
#endif

#include <stdbool.h>
#include <stdio.h>

static float get_frequency(struct note_octave *no) {
	static const float base_freqs[] = {27.5, 55.0, 110.0, 220.0, 440.0, 880.0, 1760.0, 3520.0};
	float result = base_freqs[no->octave];
	int dist = no->note;
	const float power_val = powf(2.0, 1.0 / 12.0);
	if (dist < 0) {
		for (int i = (-1) * dist; i != 0; i--) {
			result /= power_val;
		}
	} else if (dist > 0) {
		for (int i = dist; i != 0; i--) {
			result *= power_val;
		}
	}
	return result;
}

static void generate_sine(struct synth_note *note, int16_t *buf, float freq, uint32_t samples_per_sec) {
	const float tmp = 2.0 * (float) M_PI * freq;
	for (uint32_t i = 0; i < note->size; i += 2) {
		float radians = tmp * 1.0 / (float) samples_per_sec * (float) (i / 2);
		buf[i] = (int16_t) (((float) note->synth.vol_max) * sinf(radians));
		buf[i + 1] = buf[i];
	}
}

static void generate_triangle(struct synth_note *note, int16_t *buf, float freq, uint32_t samples_per_sec) {
	printf("%f\n", freq);
	const int16_t half_vol_max = note->synth.vol_max / 2;
	int32_t tmp = -1 * half_vol_max;
	float signal_period = 1.0 / freq;
	uint32_t entries_in_period = (float) samples_per_sec * signal_period + 0.5;
	printf("-> %u\n", entries_in_period);
	const int16_t increment = note->synth.vol_max / (entries_in_period / 2);
	bool rise = true;
	for (uint32_t i = 0; i < note->size; i += 2) {
		buf[i] = (int16_t) tmp;
		buf[i + 1] = buf[i];
		if (rise) {
			tmp += increment;
			if (tmp > half_vol_max) {
				tmp = 2 * half_vol_max - tmp;
				rise = false;
			}
		} else {
			tmp -= increment;
			if (tmp < -1 * half_vol_max) {
				tmp = -2 * half_vol_max - tmp;
				rise = true;
			}
		}
	}
}

static void generate_square(struct synth_note *note, int16_t *buf, float freq, uint32_t samples_per_sec) {
	const int16_t half_vol_max = note->synth.vol_max / 2;
	float signal_period = 1.0 / freq;
	uint32_t entries_in_period = samples_per_sec * signal_period + 0.5;
	bool low = false;
	int16_t tmp = half_vol_max;
	for(uint32_t i = 0; i < note->size; i += 2) {
		const bool down = ((i / 2) % entries_in_period) < (entries_in_period / 2);
		if (down && !low) {
			tmp = -1 * half_vol_max;
			low = true;
		} else if (!down && low) {
			tmp = half_vol_max;
			low = false;
		}
		buf[i] = tmp;
		buf[i + 1] = buf[i];
	}
}

static void generate_sawtooth(struct synth_note *note, int16_t *buf, float freq, uint32_t samples_per_sec) {
	const int16_t half_vol_max =  note->synth.vol_max / 2;
	int32_t tmp = -1 * half_vol_max;
	float signal_period = 1.0 / freq;
	uint32_t entries_in_period = (float) samples_per_sec * signal_period + 0.5;
	const int16_t increment = note->synth.vol_max / (entries_in_period / 2);
	for (uint32_t i = 0; i < note->size; i += 2) {
		buf[i] = (int16_t) tmp;
		buf[i + 1] = buf[i];
		tmp += increment;
		if (tmp > half_vol_max) {
			tmp = -1 * half_vol_max;
		}
	}
}

static void generate(struct synth_note *note, int16_t *buf, uint32_t samples_per_sec) {
	const float f = get_frequency(&note->note_octave);
	switch (note->synth.type) {
		case SINE:
			generate_sine(note, buf, f, samples_per_sec);
			break;
		case TRIANGLE:
			generate_triangle(note, buf, f, samples_per_sec);
			break;
		case SQUARE:
			generate_square(note, buf, f, samples_per_sec);
			break;
		case SAWTOOTH:
			generate_sawtooth(note, buf, f, samples_per_sec);
			break;
		default:
	}
}

static void calculate_size(struct synth_note *note, const uint32_t end) {
	const uint32_t attack_decay_sum = note->synth.attack + note->synth.decay;
	uint32_t size = 0;
	uint32_t pressed_size = 0;
	if (note->press < note->release) {
		pressed_size = note->release - note->press;
	} else if (note->press > note->release) {
		pressed_size = end - note->press + note->release;
	} else {
		pressed_size = end;
	}
	if (note->synth.sustain == 0) {
		if (pressed_size > attack_decay_sum) {
			size = attack_decay_sum;
		} else {
			size = pressed_size;
		}
	} else {
		size = pressed_size + note->synth.release;
	}
	note->start = note->press;
	note->stop = (note->start + size) % end;
	note->size = size;
}

int16_t *synth_fill_buffer(struct synth_note *note, uint32_t samples_per_sec, const uint32_t end) {
	int16_t *buf = NULL;
	calculate_size(note, end);
#ifdef STM32F746xx
	buf = (int16_t *) pvPortMalloc(note->size * sizeof(int16_t));
#else
	buf = (int16_t *) calloc(sizeof(int16_t), note->size);
#endif
	if (!buf) {
		while(1);
	}
	generate(note, buf, samples_per_sec);
	return buf;
}
