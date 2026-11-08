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

static void generate_sine(struct synth_note *note, float *buf, float freq, uint32_t samples_per_sec) {
	const float tmp = 2.0 * (float) M_PI * freq;
	for (uint32_t i = 0; i < note->size / 2; i++) {
		float radians = tmp / (float) samples_per_sec * (float) i;
		buf[i] = sinf(radians);
	}
}

static void generate_triangle(struct synth_note *note, float *buf, float freq, uint32_t samples_per_sec) {
	float tmp = -1.0;
	float signal_period = 1.0 / freq;
	uint32_t entries_in_period = (float) samples_per_sec * signal_period + 0.5;
	const float increment = 2.0 / ((float) entries_in_period / 2.0);
	bool rise = true;
	for (uint32_t i = 0; i < note->size / 2; i++) {
		buf[i] = tmp;
		if (rise) {
			tmp += increment;
			if (tmp > 1.0) {
				tmp = 2.0 - tmp;
				rise = false;
			}
		} else {
			tmp -= increment;
			if (tmp < -1.0) {
				tmp = -2.0 - tmp;
				rise = true;
			}
		}
	}
}

static void generate_square(struct synth_note *note, float *buf, float freq, uint32_t samples_per_sec) {
	float signal_period = 1.0 / freq;
	uint32_t entries_in_period = samples_per_sec * signal_period + 0.5;
	bool low = false;
	int16_t tmp = -1.0;
	for(uint32_t i = 0; i < note->size / 2; i++) {
		const bool down = (i % entries_in_period) < (entries_in_period / 2);
		if (down && !low) {
			tmp = -1.0;
			low = true;
		} else if (!down && low) {
			tmp = 1.0;
			low = false;
		}
		buf[i] = tmp;
	}
}

static void generate_sawtooth(struct synth_note *note, float *buf, float freq, uint32_t samples_per_sec) {
	float tmp = -1.0;
	float signal_period = 1.0 / freq;
	uint32_t entries_in_period = (float) samples_per_sec * signal_period + 0.5;
	const float increment = 2.0 / (float) entries_in_period;
	for (uint32_t i = 0; i < note->size / 2; i++) {
		buf[i] = tmp;
		tmp += increment;
		if (tmp > 1.0) {
			tmp = -1.0;
		}
	}
}

static void generate(struct synth_note *note, float *buf, uint32_t samples_per_sec) {
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

static uint32_t pressed_size(struct synth_note *note, const uint32_t end) {
	if (note->press < note->release) {
		return note->release - note->press;
	} else if (note->press > note->release) {
		return end - note->press + note->release;
	} else {
		return end;
	}
}

static void calculate_size(struct synth_note *note, const uint32_t end) {
	const uint32_t attack_decay_sum = note->synth.attack + note->synth.decay;
	uint32_t size = 0;
	note->pressed_size = pressed_size(note, end);
	if (note->synth.sustain == 0) {
		if (note->pressed_size > attack_decay_sum) {
			size = attack_decay_sum;
		} else {
			size = note->pressed_size;
		}
	} else {
		size = note->pressed_size + note->synth.release;
	}
	note->start = note->press;
	note->stop = (note->start + size) % end;
	note->size = size;
}

static float calc_dump(uint32_t i, uint32_t target) {
	return (float) i / (float) target;
}

static void modulate(struct synth_note *note, float *pattern, int16_t *buf) {
	uint32_t i = 0;
	bool no_sustain;
	if (note->synth.attack + note->synth.decay >= note->pressed_size)
		no_sustain = true;
	else if (!note->synth.sustain)
		no_sustain = true;
	else
		no_sustain = false;
	uint32_t attack;
	// TODO: no other case here?
	attack = note->synth.attack < note->pressed_size ?
		note->synth.attack : note->pressed_size;
	int16_t max_attack_vol;
	if (!attack) {
		max_attack_vol = note->synth.vol_max;
	} else {
		for (; i < attack; i += 2) {
			float dump = calc_dump(i / 2, attack / 2);
			max_attack_vol = (int16_t) roundf(dump * (float) note->synth.vol_max);
			buf[i] = (int16_t) roundf((float) max_attack_vol * pattern[i / 2]);
			buf[i + 1] = buf[i];
		}
	}
	int16_t target_volume = no_sustain ? 0 : note->synth.sustain;
	for (; i - attack < note->synth.decay; i += 2) {
		int16_t cur_decay_vol = (int16_t) roundf(
			(((float) target_volume - (float) max_attack_vol) / (float) note->synth.decay) * ((float) (i - attack)) + (float) max_attack_vol
		);
		buf[i] = (int16_t) roundf((float) cur_decay_vol * pattern[i / 2]);
		buf[i + 1] = buf[i];
	}
	if (no_sustain) {
		return;
	}
	uint32_t sustain = note->pressed_size;
	for (; i < sustain; i += 2) {
		buf[i] = (int16_t) roundf((float) note->synth.sustain * pattern[i / 2]);
		buf[i + 1] = buf[i];
	}
	if (!note->synth.release) {
		return;
	}
	int j = 0;
	for (; i < note->size; i += 2) {
		int16_t cur_vol = (int16_t) roundf(-1.0 * (float) note->synth.sustain / (float) note->synth.release * (float) j + (float) note->synth.sustain);
		buf[i] = (int16_t) roundf((float) cur_vol * pattern[i / 2]);
		buf[i + 1] = buf[i];
		j += 2;
	}
}

int16_t *synth_fill_buffer(struct synth_note *note, uint32_t samples_per_sec, const uint32_t end) {
	int16_t *buf = NULL;
	float *pattern = NULL;
	calculate_size(note, end);
#ifdef STM32F746xx
	buf = (int16_t *) pvPortMalloc(note->size * sizeof(int16_t));
	pattern = (float *) pvPortMalloc(note->size / 2 * sizeof(float));
#else
	buf = (int16_t *) calloc(sizeof(int16_t), note->size);
	pattern = (float *) calloc(sizeof(float), note->size / 2);
#endif
	if (!buf || !pattern) {
		while(1);
	}
	generate(note, pattern, samples_per_sec);
	modulate(note, pattern, buf);
#ifdef STM32F746xx
	vPortFree(pattern);
#else
	free(pattern);
#endif
	return buf;
}
