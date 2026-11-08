#include "synth.h"
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

static int make_sine() {
	struct synth_note note = {0};
	note.note_octave.note = n_A;
	note.note_octave.octave = o_4;
	note.synth.vol_max = 200;
	note.synth.type = SINE;
	note.press = 0;
	note.release = 5000;
	note.synth.attack = 1000;
	note.synth.decay = 1000;
	note.synth.release = 1000;
	note.synth.sustain = note.synth.vol_max / 2;
	int16_t *buf = synth_fill_buffer(&note, 44100, 10000);
	int fd = open("./sine.dat", O_WRONLY | O_CREAT | O_TRUNC, 0644);
	if (fd < 0) {
		perror("open()");
		return -1;
	}
	for (int i = 0; i < note.release + note.synth.release; i += 2) {
		char to_write[128] = { 0 };
		sprintf(to_write, "%d %d\n", i / 2, buf[i]);
		write(fd, to_write, strlen(to_write));
	}
	close(fd);
	free(buf);
	return 0;
}

static int make_triangle() {
	struct synth_note note = {0};
	note.note_octave.note = n_C;
	note.note_octave.octave = o_5;
	note.synth.vol_max = 200;
	note.synth.type = TRIANGLE;
	note.press = 0;
	note.release = 5000;
	note.synth.attack = 0;
	note.synth.decay = 1000;
	note.synth.release = 1000;
	note.synth.sustain = note.synth.vol_max / 2;
	int16_t *buf = synth_fill_buffer(&note, 44100, 10000);
	int fd = open("./triangle.dat", O_WRONLY | O_CREAT | O_TRUNC, 0644);
	if (fd < 0) {
		perror("open()");
		return -1;
	}
	for (int i = 0; i < note.release + note.synth.release; i += 2) {
		char to_write[128] = { 0 };
		sprintf(to_write, "%d %d\n", i / 2, buf[i]);
		write(fd, to_write, strlen(to_write));
	}
	close(fd);
	free(buf);
	return 0;
}

static int make_triangle2() {
	struct synth_note note = {0};
	note.note_octave.note = n_C;
	note.note_octave.octave = o_6;
	note.synth.vol_max = 200;
	note.synth.type = TRIANGLE;
	note.press = 0;
	note.synth.attack = 0;
	note.synth.decay = 0;
	note.synth.release = 0;
	note.synth.sustain = note.synth.vol_max / 2;
	note.release = 9000;
	int16_t *buf = synth_fill_buffer(&note, 44100, 10000);
	int fd = open("./triangle2.dat", O_WRONLY | O_CREAT | O_TRUNC, 0644);
	if (fd < 0) {
		perror("open()");
		return -1;
	}
	for (int i = 0; i < note.release; i += 2) {
		char to_write[128] = { 0 };
		sprintf(to_write, "%d %d\n", i / 2, buf[i]);
		write(fd, to_write, strlen(to_write));
	}
	close(fd);
	free(buf);
	return 0;
}

static int make_square() {
	struct synth_note note = {0};
	note.note_octave.note = n_G;
	note.note_octave.octave = o_5;
	note.synth.vol_max = 111;
	note.synth.type = SQUARE;
	note.press = 0;
	note.release = 5000;
	note.synth.sustain = note.synth.vol_max / 2;
	int16_t *buf = synth_fill_buffer(&note, 44100, 10000);
	int fd = open("./square.dat", O_WRONLY | O_CREAT | O_TRUNC, 0644);
	if (fd < 0) {
		perror("open()");
		return -1;
	}
	for (int i = 0; i < note.release; i += 2) {
		char to_write[128] = { 0 };
		sprintf(to_write, "%d %d\n", i / 2, buf[i]);
		write(fd, to_write, strlen(to_write));
	}
	close(fd);
	free(buf);
	return 0;
}

static int make_sawtooth() {
	struct synth_note note = {0};
	note.note_octave.note = n_A;
	note.note_octave.octave = o_4;
	note.synth.vol_max = 600;
	note.synth.type = SAWTOOTH;
	note.press = 0;
	note.release = 800;
	note.synth.sustain = note.synth.vol_max / 2;
	int16_t *buf = synth_fill_buffer(&note, 44100, 1000);
	int fd = open("./sawtooth.dat", O_WRONLY | O_CREAT | O_TRUNC, 0644);
	if (fd < 0) {
		perror("open()");
		return -1;
	}
	for (int i = 0; i < 500; i += 2) {
		char to_write[128] = { 0 };
		sprintf(to_write, "%d %d\n", i / 2, buf[i]);
		write(fd, to_write, strlen(to_write));
	}
	close(fd);
	free(buf);
	return 0;
}

int main(void) {
	if (make_sine()) {
		return -1;
	}
	if (make_triangle()) {
		return -1;
	}
	if (make_triangle2()) {
		return -1;
	}
	if (make_square()) {
		return -1;
	}
	if (make_sawtooth()) {
		return -1;
	}
	return 0;
}
