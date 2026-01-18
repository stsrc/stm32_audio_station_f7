#include "ts.h"

#include "lcd.h"

#include "gui_tile.h"

#include <stdbool.h>
#include <stdlib.h>

#include <cmsis_os.h>
#include <stm32746g_discovery_ts.h>

extern osMessageQId Gui_MessageId;
uint32_t ts_detected = 0;
void ts_thread(void const *arg) {
  bool recently_pressed = false;
  BSP_TS_Init(LCD_WIDTH, LCD_HEIGHT);
  uint32_t prev_wake_time = osKernelSysTick();
  while (1) {
    uint32_t sleep_delay = 5;
    TS_StateTypeDef ts_state = {0};
    BSP_TS_GetState(&ts_state);
    if (ts_state.touchDetected) {
      ts_detected = osKernelSysTick();
      if (!recently_pressed) {
        struct touched *touched = malloc(sizeof(struct touched));
        touched->x = ts_state.touchX[0];
        touched->y = ts_state.touchY[0];
        osMessagePut(Gui_MessageId, (uint32_t)touched, 0);
        //	sleep_delay = 10;
      }
      recently_pressed = true;
    } else if (recently_pressed) {
      recently_pressed = false;
      //      sleep_delay = 10;
    }
    osDelayUntil(&prev_wake_time, sleep_delay);
  }
}
