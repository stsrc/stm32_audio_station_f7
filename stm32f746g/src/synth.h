#ifndef _SYNTH_H_
#define _SYNTH_H_

#include <stdint.h>

enum type {
	SINE = 0,
	TRIANGLE,
	SQUARE,
	SAWTOOTH
};

enum note {
	n_C = -9,
	n_D_flat = -8,
	n_D = -7,
	n_E_flat = -6,
	n_E = -5,
	n_F = -4,
	n_G_flat = -3,
	n_G = -2,
	n_A_flat = -1,
	n_A = 0,
	n_B_flat = 1,
	n_B = 2,
};

enum octave {
	o_0 = 0,
	o_1,
	o_2,
	o_3,
	o_4,
	o_5,
	o_6,
	o_7,
};

struct synth_part {
	enum type type;
	uint32_t attack;
	uint32_t decay;
	int16_t sustain;
	uint32_t release;
	int16_t vol_max;
};

struct note_octave {
	enum note note;
	enum octave octave;
};

struct synth_note {
	struct note_octave note_octave;
	struct synth_part synth;
	uint32_t press;
	uint32_t release;

	// calculated in calculate_size TODO move it somewhere else later
	uint32_t start;
	uint32_t stop;
	uint32_t size;
	uint32_t pressed_size;
};

int16_t *synth_fill_buffer(struct synth_note *note, uint32_t samples_per_sec, const uint32_t end);

#endif
