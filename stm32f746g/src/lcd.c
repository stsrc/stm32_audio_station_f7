#include "lcd.h"
#include <stdbool.h>
#include <string.h>

LTDC_HandleTypeDef hltdc;

#define FB_SIZE 0x200000
#define FB_0_ADDR 0xC0400000
#define FB_1_ADDR 0xC0600000

struct layer {
  void *addr;
  bool render;
};

#define LCD_HSYNC ((uint16_t)41)
#define LCD_HBP ((uint16_t)13)
#define LCD_HFP ((uint16_t)32)
#define LCD_VSYNC ((uint16_t)10)
#define LCD_VBP ((uint16_t)2)
#define LCD_VFP ((uint16_t)2)

#define BYTES_PER_PIXEL 4

static struct layer layers[2] = {{
                                     .addr = (void *)FB_0_ADDR,
                                     .render = false,
                                 },
                                 {
                                     .addr = (void *)FB_1_ADDR,
                                     .render = false,
                                 }};

void HAL_LTDC_MspInit(LTDC_HandleTypeDef *hltdc) {
  GPIO_InitTypeDef gpio = {
      .Mode = GPIO_MODE_AF_PP,
      .Speed = GPIO_SPEED_FAST,
      .Pull = GPIO_NOPULL,
      .Alternate = GPIO_AF14_LTDC,
  };
  RCC_PeriphCLKInitTypeDef PeriphClkInitStruct = {
      .PeriphClockSelection = RCC_PERIPHCLK_LTDC,
      .PLLSAI.PLLSAIN = 192,
      .PLLSAI.PLLSAIR = 5,
      .PLLSAIDivR = RCC_PLLSAIDIVR_4,
  };

  __HAL_RCC_LTDC_CLK_ENABLE();
  __HAL_RCC_GPIOE_CLK_ENABLE();
  __HAL_RCC_GPIOG_CLK_ENABLE();
  __HAL_RCC_GPIOI_CLK_ENABLE();
  __HAL_RCC_GPIOJ_CLK_ENABLE();
  __HAL_RCC_GPIOK_CLK_ENABLE();

  HAL_RCCEx_PeriphCLKConfig(&PeriphClkInitStruct);

  gpio.Pin = GPIO_PIN_4;
  HAL_GPIO_Init(GPIOE, &gpio);

  gpio.Pin = GPIO_PIN_12;
  gpio.Alternate = GPIO_AF9_LTDC;
  HAL_GPIO_Init(GPIOG, &gpio);

  gpio.Pin = GPIO_PIN_9 | GPIO_PIN_10 | GPIO_PIN_13 | GPIO_PIN_14 | GPIO_PIN_15;
  gpio.Alternate = GPIO_AF14_LTDC;
  HAL_GPIO_Init(GPIOI, &gpio);

  gpio.Pin = GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3 | GPIO_PIN_4 |
             GPIO_PIN_5 | GPIO_PIN_6 | GPIO_PIN_7 | GPIO_PIN_8 | GPIO_PIN_9 |
             GPIO_PIN_10 | GPIO_PIN_11 | GPIO_PIN_13 | GPIO_PIN_14 |
             GPIO_PIN_15;
  HAL_GPIO_Init(GPIOJ, &gpio);

  gpio.Pin = GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_4 | GPIO_PIN_5 |
             GPIO_PIN_6 | GPIO_PIN_7;
  HAL_GPIO_Init(GPIOK, &gpio);

  gpio.Pin = GPIO_PIN_12;
  gpio.Mode = GPIO_MODE_OUTPUT_PP;
  HAL_GPIO_Init(GPIOI, &gpio);

  gpio.Pin = GPIO_PIN_3;
  HAL_GPIO_Init(GPIOK, &gpio);

  HAL_NVIC_SetPriority(LTDC_IRQn, 0xE, 0);

  HAL_NVIC_EnableIRQ(LTDC_IRQn);
}

void HAL_LTDC_MspDeInit(LTDC_HandleTypeDef *hltdc) {
  __HAL_RCC_LTDC_FORCE_RESET();
  __HAL_RCC_LTDC_RELEASE_RESET();
}

void HAL_LTDC_LineEventCallback(LTDC_HandleTypeDef *hltdc) {
  for (int i = 0; i < TOT_LAYERS; i++) {
    if (layers[i].render) {
      __HAL_LTDC_LAYER(hltdc, i)->CFBAR = (uint32_t)layers[i].addr;
      __HAL_LTDC_RELOAD_CONFIG(hltdc);
      layers[i].render = false;
    }
  }
  HAL_LTDC_ProgramLineEvent(hltdc, 0);
}

void LCD_Init(void) {
  __HAL_RCC_DMA2D_CLK_ENABLE();
  HAL_LTDC_DeInit(&hltdc);
  hltdc.Init.HorizontalSync = (LCD_HSYNC - 1);
  hltdc.Init.VerticalSync = (LCD_VSYNC - 1);
  hltdc.Init.AccumulatedHBP = (LCD_HSYNC + LCD_HBP - 1);
  hltdc.Init.AccumulatedVBP = (LCD_VSYNC + LCD_VBP - 1);
  hltdc.Init.AccumulatedActiveH = (LCD_HEIGHT + LCD_VSYNC + LCD_VBP - 1);
  hltdc.Init.AccumulatedActiveW = (LCD_WIDTH + LCD_HSYNC + LCD_HBP - 1);
  hltdc.Init.TotalHeigh = (LCD_HEIGHT + LCD_VSYNC + LCD_VBP + LCD_VFP - 1);
  hltdc.Init.TotalWidth = (LCD_WIDTH + LCD_HSYNC + LCD_HBP + LCD_HFP - 1);
  hltdc.Init.Backcolor.Blue = 0;
  hltdc.Init.Backcolor.Green = 0;
  hltdc.Init.Backcolor.Red = 0;
  hltdc.Init.HSPolarity = LTDC_HSPOLARITY_AL;
  hltdc.Init.VSPolarity = LTDC_VSPOLARITY_AL;
  hltdc.Init.DEPolarity = LTDC_DEPOLARITY_AL;
  hltdc.Init.PCPolarity = LTDC_PCPOLARITY_IPC;
  hltdc.Instance = LTDC;
  HAL_LTDC_Init(&hltdc);
  HAL_LTDC_ProgramLineEvent(&hltdc, 0);
  HAL_LTDC_EnableDither(&hltdc);
  HAL_GPIO_WritePin(GPIOI, GPIO_PIN_12, GPIO_PIN_SET);
  HAL_GPIO_WritePin(GPIOK, GPIO_PIN_3, GPIO_PIN_SET);

  LTDC_LayerCfgTypeDef config;
  config.Alpha = 255;
  config.Alpha0 = 0;
  config.Backcolor.Blue = 0;
  config.Backcolor.Green = 0;
  config.Backcolor.Red = 0;
  config.BlendingFactor1 = LTDC_BLENDING_FACTOR1_PAxCA;
  config.BlendingFactor2 = LTDC_BLENDING_FACTOR2_PAxCA;
  config.PixelFormat = LTDC_PIXEL_FORMAT_ARGB8888;
  config.ImageWidth = LCD_WIDTH;
  config.ImageHeight = LCD_HEIGHT;
  config.WindowX0 = 0;
  config.WindowX1 = LCD_WIDTH;
  config.WindowY0 = 0;
  config.WindowY1 = LCD_HEIGHT;
  for (int i = 0; i < TOT_LAYERS; i++) {
    memset((void *)layers[i].addr, 0, FB_SIZE);
    config.FBStartAdress = (uint32_t)layers[i].addr;
    HAL_LTDC_ConfigLayer(&hltdc, &config, i);
  }
}

static void DMA2D_FillBuffer(enum Layer index, void *addr, uint32_t width,
                             uint32_t height, uint32_t offset, uint32_t color) {

  DMA2D->OCOLR = color;
  DMA2D->CR = 0x00030000UL | (1 << 9);
  DMA2D->OMAR = (uint32_t)addr;
  DMA2D->OOR = offset;
  DMA2D->OPFCCR = LTDC_PIXEL_FORMAT_ARGB8888;
  DMA2D->NLR = (uint32_t)((width << 16) | (uint16_t)height);
  DMA2D->CR |= DMA2D_CR_START;
  while (DMA2D->CR & DMA2D_CR_START) {
    // busywait
  }
  LCD_mark_to_render(index);
}

void LCD_mark_to_render(enum Layer index) { layers[index].render = true; }

void LCD_Fill_Rect(enum Layer index, uint32_t x, uint32_t y, uint32_t width,
                   uint32_t height, uint32_t color) {
  void *addr = layers[index].addr + (x + y * LCD_WIDTH) * BYTES_PER_PIXEL;
  uint32_t offset = LCD_WIDTH - width;
  DMA2D_FillBuffer(index, addr, width, height, offset, color);
}
