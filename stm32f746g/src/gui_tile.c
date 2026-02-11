#include "gui_tile.h"

#include <play.h>
#include <sample.h>
#include <ts.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <stm32746g_discovery_lcd.h>

#define LCD_WIDTH 480
#define LCD_HEIGHT 272
#define FB_SIZE 0x200000
#define FB_0_ADDR 0xC0400000
#define FB_1_ADDR 0xC0600000

#define BYTES_PER_PIXEL 4

static struct tile head = {0};

static uint8_t current_level = 0;

osMessageQId Gui_MessageId;

static uint32_t BPM = 85;
#define BPM_MIN 0
#define BPM_MAX 1000

void gui_tile_add(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1,
                  void (*action)(struct tile *), uint8_t level,
                  void (*draw)(struct tile *), void *priv,
                  void (*init)(struct tile *)) {
  static uint8_t last_level = 0;
  if (x0 > x1 || y0 > y1 || !draw || last_level > level) {
    printf("%s(): wrong arguments\n", __func__);
    return;
  }
  last_level = level;
  struct tile *tmp = &head;
  while (tmp->next) {
    tmp = tmp->next;
  }
  tmp->next = calloc(1, sizeof(struct tile));
  tmp = tmp->next;
  tmp->x0 = x0;
  tmp->y0 = y0;
  tmp->x1 = x1;
  tmp->y1 = y1;
  tmp->level = level;
  tmp->action = action;
  tmp->draw = draw;
  tmp->priv = priv;

  if (init) {
    init(tmp);
  }
}

static struct tile *gui_tile_search(uint16_t x, uint16_t y) {
  struct tile *tmp = head.next;
  while (tmp) {
    if (x >= tmp->x0 && x <= tmp->x1 && y >= tmp->y0 && y <= tmp->y1 &&
        current_level == tmp->level) {
      return tmp;
    }
    tmp = tmp->next;
  }
  return NULL;
}

void gui_tile_draw_all() {
  struct tile *tmp = head.next;
  memset((void *)FB_0_ADDR, 0, FB_SIZE);
  BSP_LCD_SelectLayer(0);
  while (tmp && tmp->draw) {
    if (current_level == tmp->level) {
      tmp->draw(tmp);
    }
    tmp = tmp->next;
  }
  BSP_LCD_Reload(LCD_RELOAD_VERTICAL_BLANKING);
}

void gui_tile_set_active_level(uint8_t level) { current_level = level; }

void gui_tile_sample_draw(struct tile *tile) {
  BSP_LCD_SetTextColor(0xffaaaaaa);
  BSP_LCD_SetBackColor(0xff000000);
  BSP_LCD_DrawRect(tile->x0, tile->y0, tile->x1 - tile->x0,
                   tile->y1 - tile->y0);
  sFONT *font = &Font16;
  BSP_LCD_SetFont(font);
  uint32_t x_delta = tile->x1 - tile->x0;
  uint32_t y_delta = tile->y1 - tile->y0;
  uint32_t text_width = font->Width * strlen(tile->priv);
  uint32_t text_height = font->Height;
  if (x_delta < text_width || y_delta < text_height) {
	  printf("Tile too small for a text\n");
	  return;
  }
  text_width /= 2;
  text_height /= 2;
  x_delta /= 2;
  y_delta /= 2;
  BSP_LCD_DisplayStringAt(tile->x0 + (x_delta - text_width),
                          tile->y0 + (y_delta - text_height),
                          (uint8_t *)tile->priv, LEFT_MODE);
}

void gui_tile_current_bpm_draw(struct tile *tile) {
  BSP_LCD_SetTextColor(0xffaaaaaa);
  BSP_LCD_SetBackColor(0xff000000);
  BSP_LCD_SetFont(&Font16);
  char buf[16] = {0};
  snprintf(buf, sizeof(buf)/sizeof(char), "BPM: %lu", BPM);
  BSP_LCD_DisplayStringAt(tile->x0 + 4,
		  tile->y0 + (tile->y1 - tile->y0) / 2 - 4,
			  (uint8_t *)buf, LEFT_MODE);
}

void gui_tile_play_draw(struct tile *tile) {
  BSP_LCD_SetTextColor(0xffaaaaaa);
  BSP_LCD_SetBackColor(0xff000000);
  BSP_LCD_DrawRect(tile->x0, tile->y0, tile->x1 - tile->x0,
                   tile->y1 - tile->y0);
  BSP_LCD_DrawLine(tile->x0 + 5, tile->y0 + 5, tile->x0 + 5, tile->y1 - 5);
  BSP_LCD_DrawLine(tile->x0 + 5, tile->y0 + 5, tile->x1 - 5,
                   tile->y0 + (tile->y1 - tile->y0) / 2);
  BSP_LCD_DrawLine(tile->x0 + 5, tile->y1 - 5, tile->x1 - 5,
                   tile->y0 + (tile->y1 - tile->y0) / 2);
}

void gui_tile_pause_draw(struct tile *tile) {
  BSP_LCD_SetTextColor(0xffaaaaaa);
  BSP_LCD_SetBackColor(0xff000000);
  uint32_t x_width = tile->x1 - tile->x0;
  uint32_t y_width = tile->y1 - tile->y0;
  uint32_t x_half_pos = tile->x0 + x_width / 2;
  BSP_LCD_DrawRect(tile->x0, tile->y0, x_width, y_width);
  BSP_LCD_DrawRect(x_half_pos - 6, tile->y0 + 10, 4, y_width - 20);
  BSP_LCD_DrawRect(x_half_pos + 2, tile->y0 + 10, 4, y_width - 20);
}

void gui_tile_metronome_draw(struct tile *tile) {
  BSP_LCD_SetTextColor(0xffaaaaaa);
  BSP_LCD_SetBackColor(0xff000000);
  BSP_LCD_DrawRect(tile->x0, tile->y0, tile->x1 - tile->x0,
                   tile->y1 - tile->y0);
  BSP_LCD_DrawLine(tile->x0 + 5, tile->y1 - 5, tile->x1 - 5, tile->y1 - 5);
  uint32_t x_width = tile->x1 - tile->x0;
  uint32_t x_half_pos = tile->x0 + x_width / 2;
  BSP_LCD_DrawLine(x_half_pos, tile->y0 + 5, x_half_pos - 10, tile->y1 - 5);
  BSP_LCD_DrawLine(x_half_pos, tile->y0 + 5, x_half_pos + 10, tile->y1 - 5);
  BSP_LCD_DrawLine(x_half_pos + 15, tile->y0 + 10, x_half_pos, tile->y1 - 10);
}

void gui_tile_sample_action(struct tile *tile) {
  struct play_message *msg = malloc(sizeof(struct play_message));
  if (!msg) {
    return;
  } else if (!tile->priv) {
    free(msg);
    return;
  }
  msg->op = ADD;
  msg->data.name = (const char *)tile->priv;
  osMessagePut(Play_MessageId, (uint32_t)msg, 0);
}

void gui_tile_sample_load(struct tile *tile) {
  sample_open((const char *)tile->priv);
}

void gui_tile_stop_action(struct tile *tile) {
  gui_tile_clear_action(NULL);
  struct play_message *msg = calloc(1, sizeof(struct play_message));
  if (!msg) {
    return;
  }
  msg->op = STOP;
  osMessagePut(Play_MessageId, (uint32_t)msg, 0);
}

void gui_tile_play_action(struct tile *tile) {
  struct play_message *msg = calloc(1, sizeof(struct play_message));
  if (!msg) {
    return;
  }
  msg = calloc(1, sizeof(struct play_message));
  msg->op = SETUP;
  msg->data.settings.BPM = BPM;
  msg->data.settings.samples_per_sec = 44100;
  osMessagePut(Play_MessageId, (uint32_t)msg, 0);
}

void gui_tile_metronome_action(struct tile *tile) {
  struct play_message *msg = malloc(sizeof(struct play_message));
  if (!msg) {
    printf("No mem\n");
    return;
  }
  msg->op = METRONOME;
  msg->data.name = (const char *)tile->priv;
  osMessagePut(Play_MessageId, (uint32_t)msg, 0);
}

void gui_tile_clear_action(struct tile *tile) {
  struct play_message *msg = malloc(sizeof(struct play_message));
  if (!msg) {
    printf("No mem\n");
    return;
  }
  msg->op = DELETE_ALL;
  osMessagePut(Play_MessageId, (uint32_t)msg, 0);
}

void gui_tile_pause_action(struct tile *tile) {
  struct play_message *msg = malloc(sizeof(struct play_message));
  if (!msg) {
    return;
  }
  msg->op = PAUSE;
  osMessagePut(Play_MessageId, (uint32_t)msg, 0);
}

void gui_tile_bpm_up_action(struct tile *tile) {
	if (BPM < BPM_MAX)
	  BPM++;
	gui_tile_draw_all();
}

void gui_tile_bpm_down_action(struct tile *tile) {
	if (BPM > BPM_MIN)
	  BPM--;
	gui_tile_draw_all();
}

void gui_tile_setup_action(struct tile *tile) {
	current_level++;
	gui_tile_draw_all();
}

void gui_tile_back_action(struct tile *tile) {
	if (!current_level) {
		while(1);
	}
	current_level--;
	gui_tile_draw_all();
	gui_tile_stop_action(NULL);
	gui_tile_play_action(NULL);
}

void gui_tiles_thread(void const *arg) {
  BSP_LCD_Init();
  BSP_LCD_SetXSize(LCD_WIDTH);
  BSP_LCD_SetYSize(LCD_HEIGHT);
  memset((void *)FB_0_ADDR, 0, FB_SIZE);
  memset((void *)FB_1_ADDR, 0, FB_SIZE);
  BSP_LCD_LayerDefaultInit(0, FB_0_ADDR);
  BSP_LCD_LayerDefaultInit(1, FB_1_ADDR);
  BSP_LCD_DisplayOn();
  gui_tile_draw_all();
  while (1) {
    osEvent evt = osMessageGet(Gui_MessageId, osWaitForever);
    struct touched *touched = (struct touched *)evt.value.p;
    struct tile *tile = gui_tile_search(touched->x, touched->y);
    if (tile && tile->action) {
      tile->action(tile);
    }
    free(touched);
  }
}
