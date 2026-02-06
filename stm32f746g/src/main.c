#include <stm32f7xx.h>
#include <core_cm7.h>

#include <stm32f7xx_hal.h>
#include <stm32f7xx_it.h>

#include <stm32746g_discovery.h>
#include <stm32746g_discovery_qspi.h>
#include <stm32746g_discovery_sdram.h>

#include "gui_tile.h"
#include "ts.h"

#include "play.h"
#include "sample.h"

#include <cmsis_os.h>
#include <stdlib.h>
#include <string.h>

static void k_BspInit(void) {
  /* Initialize the NOR */
  BSP_QSPI_Init();
  BSP_QSPI_MemoryMappedMode();

  /* Initialize the SDRAM */
  BSP_SDRAM_Init();

  /* Enable CRC to Unlock GUI */
  __HAL_RCC_CRC_CLK_ENABLE();

  /* Enable Back up SRAM */
  __HAL_RCC_BKPSRAM_CLK_ENABLE();
}
static void SystemClock_Config(void) {
  RCC_ClkInitTypeDef RCC_ClkInitStruct;
  RCC_OscInitTypeDef RCC_OscInitStruct;
  RCC_PeriphCLKInitTypeDef PeriphClkInitStruct;
  HAL_StatusTypeDef ret = HAL_OK;

  /* Enable HSE Oscillator and activate PLL with HSE as source */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 25;
  RCC_OscInitStruct.PLL.PLLN = 432;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 9;

  ret = HAL_RCC_OscConfig(&RCC_OscInitStruct);
  if (ret != HAL_OK) {
    while (1) {
      ;
    }
  }

  /* Activate the OverDrive to reach the 200 MHz Frequency */
  ret = HAL_PWREx_EnableOverDrive();
  if (ret != HAL_OK) {
    while (1) {
      ;
    }
  }

  /* Select PLLSAI output as USB clock source */
  PeriphClkInitStruct.PeriphClockSelection = RCC_PERIPHCLK_CLK48;
  PeriphClkInitStruct.Clk48ClockSelection = RCC_CLK48SOURCE_PLLSAIP;
  PeriphClkInitStruct.PLLSAI.PLLSAIN = 192;
  PeriphClkInitStruct.PLLSAI.PLLSAIQ = 4;
  PeriphClkInitStruct.PLLSAI.PLLSAIP = RCC_PLLSAIP_DIV4;
  ret = HAL_RCCEx_PeriphCLKConfig(&PeriphClkInitStruct);
  if (ret != HAL_OK) {
    while (1) {
      ;
    }
  }

  /* Select PLL as system clock source and configure the HCLK, PCLK1 and PCLK2
   * clocks dividers */
  RCC_ClkInitStruct.ClockType = (RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_HCLK |
                                 RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2);
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;

  ret = HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_7);
  if (ret != HAL_OK) {
    while (1) {
      ;
    }
  }
}

static void MPU_Config(void) {
  MPU_Region_InitTypeDef MPU_InitStruct;

  /* Disable the MPU */
  HAL_MPU_Disable();

  /* Configure the MPU as Strongly ordered for not defined regions */
  MPU_InitStruct.Enable = MPU_REGION_ENABLE;
  MPU_InitStruct.BaseAddress = 0x00;
  MPU_InitStruct.Size = MPU_REGION_SIZE_4GB;
  MPU_InitStruct.AccessPermission = MPU_REGION_NO_ACCESS;
  MPU_InitStruct.IsBufferable = MPU_ACCESS_NOT_BUFFERABLE;
  MPU_InitStruct.IsCacheable = MPU_ACCESS_NOT_CACHEABLE;
  MPU_InitStruct.IsShareable = MPU_ACCESS_SHAREABLE;
  MPU_InitStruct.Number = MPU_REGION_NUMBER0;
  MPU_InitStruct.TypeExtField = MPU_TEX_LEVEL0;
  MPU_InitStruct.SubRegionDisable = 0x87;
  MPU_InitStruct.DisableExec = MPU_INSTRUCTION_ACCESS_DISABLE;

  HAL_MPU_ConfigRegion(&MPU_InitStruct);

  /* Configure the MPU as Normal Non Cacheable for the SRAM1 and SRAM2 */
  MPU_InitStruct.Enable = MPU_REGION_ENABLE;
  MPU_InitStruct.BaseAddress = 0x20010000;
  MPU_InitStruct.Size = MPU_REGION_SIZE_256KB;
  MPU_InitStruct.AccessPermission = MPU_REGION_FULL_ACCESS;
  MPU_InitStruct.IsBufferable = MPU_ACCESS_NOT_BUFFERABLE;
  MPU_InitStruct.IsCacheable = MPU_ACCESS_NOT_CACHEABLE;
  MPU_InitStruct.IsShareable = MPU_ACCESS_SHAREABLE;
  MPU_InitStruct.Number = MPU_REGION_NUMBER1;
  MPU_InitStruct.TypeExtField = MPU_TEX_LEVEL1;
  MPU_InitStruct.SubRegionDisable = 0x00;
  MPU_InitStruct.DisableExec = MPU_INSTRUCTION_ACCESS_ENABLE;

  HAL_MPU_ConfigRegion(&MPU_InitStruct);

  /* Configure the MPU attributes as WT for SDRAM */
  MPU_InitStruct.Enable = MPU_REGION_ENABLE;
  MPU_InitStruct.BaseAddress = 0xC0000000;
  MPU_InitStruct.Size = MPU_REGION_SIZE_32MB;
  MPU_InitStruct.AccessPermission = MPU_REGION_FULL_ACCESS;
  MPU_InitStruct.IsBufferable = MPU_ACCESS_NOT_BUFFERABLE;
  MPU_InitStruct.IsCacheable = MPU_ACCESS_CACHEABLE;
  MPU_InitStruct.IsShareable = MPU_ACCESS_NOT_SHAREABLE;
  MPU_InitStruct.Number = MPU_REGION_NUMBER2;
  MPU_InitStruct.TypeExtField = MPU_TEX_LEVEL0;
  MPU_InitStruct.SubRegionDisable = 0x00;
  MPU_InitStruct.DisableExec = MPU_INSTRUCTION_ACCESS_ENABLE;

  HAL_MPU_ConfigRegion(&MPU_InitStruct);

  /* Configure the MPU QSPI flash */
  MPU_InitStruct.Enable = MPU_REGION_ENABLE;
  MPU_InitStruct.BaseAddress = 0x90000000;
  MPU_InitStruct.Size = MPU_REGION_SIZE_64MB;
  MPU_InitStruct.AccessPermission = MPU_REGION_FULL_ACCESS;
  MPU_InitStruct.IsBufferable = MPU_ACCESS_NOT_BUFFERABLE;
  MPU_InitStruct.IsCacheable = MPU_ACCESS_CACHEABLE;
  MPU_InitStruct.IsShareable = MPU_ACCESS_NOT_SHAREABLE;
  MPU_InitStruct.Number = MPU_REGION_NUMBER3;
  MPU_InitStruct.TypeExtField = MPU_TEX_LEVEL0;
  MPU_InitStruct.SubRegionDisable = 0x0;
  MPU_InitStruct.DisableExec = MPU_INSTRUCTION_ACCESS_ENABLE;

  HAL_MPU_ConfigRegion(&MPU_InitStruct);

  /* Configure the MPU attributes FMC control registers */
  MPU_InitStruct.Enable = MPU_REGION_ENABLE;
  MPU_InitStruct.BaseAddress = 0xA0000000;
  MPU_InitStruct.Size = MPU_REGION_SIZE_8KB;
  MPU_InitStruct.AccessPermission = MPU_REGION_FULL_ACCESS;
  MPU_InitStruct.IsBufferable = MPU_ACCESS_BUFFERABLE;
  MPU_InitStruct.IsCacheable = MPU_ACCESS_NOT_CACHEABLE;
  MPU_InitStruct.IsShareable = MPU_ACCESS_SHAREABLE;
  MPU_InitStruct.Number = MPU_REGION_NUMBER4;
  MPU_InitStruct.TypeExtField = MPU_TEX_LEVEL0;
  MPU_InitStruct.SubRegionDisable = 0x0;
  MPU_InitStruct.DisableExec = MPU_INSTRUCTION_ACCESS_DISABLE;

  HAL_MPU_ConfigRegion(&MPU_InitStruct);

  /* Enable the MPU */
  HAL_MPU_Enable(MPU_PRIVILEGED_DEFAULT);
}

#ifdef USE_FULL_ASSERT
/**
 * @brief  assert_failed
 *         Reports the name of the source file and the source line number
 *         where the assert_param error has occurred.
 * @param  File: pointer to the source file name
 * @param  Line: assert_param error line source number
 * @retval None
 */
void assert_failed(uint8_t *file, uint32_t line) {
  /* User can add his own implementation to report the file name and line
  number,ex: printf("Wrong parameters value: file %s on line %d\r\n",
  file, line) */

  /* Infinite loop */
  while (1) {
  }
}

#endif

osThreadId GUI_ThreadId;
osThreadId Play_ThreadId;
osThreadId TouchScreen_ThreadId;

static const HeapRegion_t xHeapRegions[] = {
    {(uint8_t *)0xC0000000, 0x400000}, // Region 1: 4 MB
    {NULL, 0}                          // Terminator
};

int main(void) {
  /* Configure the MPU attributes */
  MPU_Config();

  /* Invalidate I-Cache : ICIALLU register */
  SCB_InvalidateICache();

  /* Enable branch prediction */
  SCB->CCR |= (1 << 18);
  __DSB();

  /* Invalidate I-Cache : ICIALLU register */
  SCB_InvalidateICache();

  /* Enable I-Cache */
  SCB_EnableICache();

  SCB_InvalidateDCache();
  SCB_EnableDCache();

  /* STM32F7xx HAL library initialization:
  - Configure the Flash ART accelerator on ITCM interface
  - Configure the Systick to generate an interrupt each 1 msec
  - Set NVIC Group Priority to 4
  - Global MSP (MCU Support Package) initialization
  */
  HAL_Init();

  /* Configure the system clock @ 200 Mhz */
  SystemClock_Config();

  /* Configure the board */
  k_BspInit();

  vPortDefineHeapRegions(xHeapRegions);

  osMessageQDef(Gui, 8, uint32_t);
  osMessageQDef(Play, 8, uint32_t);

  Gui_MessageId = osMessageCreate(osMessageQ(Gui), NULL);
  Play_MessageId = osMessageCreate(osMessageQ(Play), NULL);

  if (!sample_init()) {
    while (1)
      ;
  }

  gui_tile_add(10, 10, 60, 60, gui_tile_play_action, 0, gui_tile_play_draw,
               NULL, NULL);

  gui_tile_add(10, 65, 60, 115, gui_tile_pause_action, 0, gui_tile_pause_draw,
               NULL, NULL);

  gui_tile_add(10, 120, 60, 170, gui_tile_metronome_action, 0,
               gui_tile_metronome_draw, (void *)"tick.wav",
               gui_tile_sample_load);

  gui_tile_add(10, 175, 60, 225, gui_tile_clear_action, 0, gui_tile_sample_draw,
               (void *)"CLR", NULL);

  gui_tile_add(70, 10, 135, 75, gui_tile_sample_action, 0, gui_tile_sample_draw,
               (void *)"A.wav", gui_tile_sample_load);

  gui_tile_add(140, 10, 205, 75, gui_tile_sample_action, 0,
               gui_tile_sample_draw, (void *)"B.wav", gui_tile_sample_load);

  gui_tile_add(210, 10, 275, 75, gui_tile_sample_action, 0,
               gui_tile_sample_draw, (void *)"C.wav", gui_tile_sample_load);

  gui_tile_add(280, 10, 345, 75, gui_tile_sample_action, 0,
               gui_tile_sample_draw, (void *)"D.wav", gui_tile_sample_load);

  osThreadDef(GUI, gui_tiles_thread, osPriorityNormal, 0,
              4 * configMINIMAL_STACK_SIZE);
  GUI_ThreadId = osThreadCreate(osThread(GUI), NULL);
  if (!GUI_ThreadId) {
    while (1)
      ;
  }

  osThreadDef(TouchScreen, ts_thread, osPriorityNormal, 0,
              configMINIMAL_STACK_SIZE);
  TouchScreen_ThreadId = osThreadCreate(osThread(TouchScreen), NULL);
  if (!TouchScreen_ThreadId) {
    while (1)
      ;
  }

  osThreadDef(Play, play_thread, osPriorityNormal, 0,
              8 * configMINIMAL_STACK_SIZE);
  Play_ThreadId = osThreadCreate(osThread(Play), NULL);
  if (!Play_ThreadId) {
    while (1)
      ;
  }

  osKernelStart();

  while (1)
    ;
  return 0;
}
