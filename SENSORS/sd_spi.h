#ifndef SD_SPI_H
#define SD_SPI_H

#include "main.h"
#include <stdint.h>

#define SD_CARD_CS_PIN      GPIO_PIN_0
#define SD_CARD_CS_PORT     GPIOA

#define SD_BLOCK_SIZE  512

int  SD_SPI_Init(void);
int  SD_SPI_ReadBlock(uint32_t block, uint8_t *buf);
int  SD_SPI_WriteBlock(uint32_t block, const uint8_t *buf);
uint8_t SD_GetLastCmd0Resp(void);

#endif
