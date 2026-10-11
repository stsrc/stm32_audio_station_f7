#include "synth.h"

#include <unity.h>

#include <stdlib.h>

void setUp(void) {

}

void tearDown(void) {

}

void test_buffer_size(void) {
        struct synth_note note = {0};
        note.note_octave.note = n_A;
        note.note_octave.octave = o_4;
        note.synth.vol_max = 256;
        note.synth.type = SINE;
        note.press = 0;
	const uint32_t length = 500;
        note.release = length;
        note.synth.sustain = note.synth.vol_max / 2;
        int16_t *buf = synth_fill_buffer(&note, 44100, 1000);
	TEST_ASSERT_EQUAL(note.start, 0);
	TEST_ASSERT_EQUAL(note.stop, length);
	TEST_ASSERT_EQUAL(note.size, length);
	free(buf);
}

int main(void) {
	UNITY_BEGIN();
	RUN_TEST(test_buffer_size);
	return UNITY_END();
}
