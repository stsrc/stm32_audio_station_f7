#ifndef _GUI_TILE_H_
#define _GUI_TILE_H_

#include <cmsis_os.h>
#include <stdint.h>

extern osMessageQId Gui_MessageId;

struct touched {
  uint16_t x;
  uint16_t y;
};

struct tile {
  uint16_t x0, y0, x1, y1;
  void (*action)(struct tile *);
  void (*draw)(struct tile *);
  uint8_t level;
  struct tile *next;

  void *priv;
};

void gui_tile_add(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1,
                  void (*action)(struct tile *), uint8_t level,
                  void (*draw)(struct tile *), void *priv,
                  void (*init)(struct tile *));

void gui_tile_draw_all();

void gui_tile_set_active_level(uint8_t level);

void gui_tiles_thread(void const *arg);

void gui_tile_sample_draw(struct tile *tile);
void gui_tile_sample_action(struct tile *tile);
void gui_tile_sample_load(struct tile *tile);

void gui_tile_play_draw(struct tile *tile);
void gui_tile_play_action(struct tile *tile);

void gui_tile_pause_draw(struct tile *tile);
void gui_tile_pause_action(struct tile *tile);

void gui_tile_metronome_draw(struct tile *tile);
void gui_tile_metronome_action(struct tile *tile);

void gui_tile_clear_action(struct tile *tile);

#endif
