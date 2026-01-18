#include "sd_diskio.h"

#include <stm32746g_discovery_sd.h>

#include <stdio.h>

#define TIMEOUT 30 * 1000

#define DEFAULT_BLOCK_SIZE 512

static DSTATUS sd_status(BYTE flag) {
  uint8_t bsp_ret = BSP_SD_GetCardState();
  if (bsp_ret) {
    printf("Error while getting sd card state: %hu\n", (uint16_t)bsp_ret);
    return STA_NOINIT;
  }
  return RES_OK;
}

static DSTATUS sd_initialize(BYTE flag) {
  uint8_t bsp_ret = BSP_SD_Init();
  if (bsp_ret) {
    printf("Error initializing sd card: %hu\n", (uint16_t)bsp_ret);
    return STA_NOINIT;
  }
  return sd_status(flag);
}

static DRESULT sd_read(BYTE lun, BYTE *buf, DWORD sector, UINT count) {
  uint8_t bsp_ret =
      BSP_SD_ReadBlocks((uint32_t *)buf, (uint32_t)sector, count, TIMEOUT);
  if (bsp_ret) {
    printf("Error reading sd card: %hu\n", bsp_ret);
    return RES_ERROR;
  }
  while (BSP_SD_GetCardState() != MSD_OK)
    ;
  return RES_OK;
}

#if _USE_WRITE == 1
static DRESULT sd_write(BYTE lun, const BYTE *buf, DWORD sector, UINT count) {
  uint8_t bsp_ret =
      BSP_SD_WriteBlocks((uint32_t *)buf, (uint32_t)sector, count, TIMEOUT);
  if (bsp_ret) {
    printf("Error reading sd card: %hu\n", bsp_ret);
    return RES_ERROR;
  }
  while (BSP_SD_GetCardState() != MSD_OK)
    ;
  return RES_OK;
}
#endif

#if _USE_IOCTL == 1
static DRESULT sd_ioctl(BYTE lun, BYTE cmd, void *buf) {
  BSP_SD_CardInfo info = {0};
  uint8_t bsp_ret = BSP_SD_GetCardState();
  if (bsp_ret) {
    printf("Error getting sd card state: %hu\n", bsp_ret);
    return RES_ERROR;
  }
  switch (cmd) {
  case CTRL_SYNC:
    return RES_OK;
  case GET_SECTOR_COUNT:
    BSP_SD_GetCardInfo(&info);
    *(DWORD *)buf = info.LogBlockNbr;
    return RES_OK;
  case GET_SECTOR_SIZE:
    BSP_SD_GetCardInfo(&info);
    *(DWORD *)buf = info.LogBlockSize;
    return RES_OK;
  case GET_BLOCK_SIZE:
    BSP_SD_GetCardInfo(&info);
    *(DWORD *)buf = info.LogBlockSize / DEFAULT_BLOCK_SIZE;
    return RES_OK;
  default:
    return RES_PARERR;
  }
  return RES_ERROR;
}
#endif

const Diskio_drvTypeDef sd_op = {
    .disk_initialize = sd_initialize,
    .disk_status = sd_status,
    .disk_read = sd_read,
#if _USE_WRITE == 1
    .disk_write = sd_write,
#endif
#if _USE_IOCTL == 1
    .disk_ioctl = sd_ioctl,
#endif
};
