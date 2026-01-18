/**
  ******************************************************************************
  * @file    Audio/Audio_playback_and_record/Src/stm32f7xx_it.c
  * @author  MCD Application Team
  * @brief   Main Interrupt Service Routines.
  *          This file provides template for all exceptions handler and
  *          peripherals interrupt service routine.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2016 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
*/

#include "stm32f7xx_it.h"
#include <stm32f7xx.h>

#include <cmsis_os.h>
#include <core_cm7.h>
#include <stm32f7xx_hal.h>

extern SAI_HandleTypeDef haudio_out_sai;
extern SD_HandleTypeDef uSdHandle;

void NMI_Handler(void) {}

void HardFault_Handler(void) {
  while (1) {
  }
}

void MemManage_Handler(void) {
  while (1) {
  }
}

void BusFault_Handler(void) {
  while (1) {
  }
}

void UsageFault_Handler(void) {
  while (1) {
  }
}

void DebugMon_Handler(void) {}

void SysTick_Handler(void) { osSystickHandler(); }

void EXTI9_5_IRQHandler(void) { HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_8); }

void DMA2_Stream4_IRQHandler(void) {
  HAL_DMA_IRQHandler(haudio_out_sai.hdmatx);
}

void BSP_SDMMC_IRQHandler(void) { HAL_SD_IRQHandler(&uSdHandle); }

void BSP_SDMMC_DMA_Tx_IRQHandler(void) { HAL_DMA_IRQHandler(uSdHandle.hdmatx); }

void BSP_SDMMC_DMA_Rx_IRQHandler(void) { HAL_DMA_IRQHandler(uSdHandle.hdmarx); }
