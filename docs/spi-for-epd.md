# SPI with the nucleo board stm32wb55-rg
User Manual: um2819-stm32wb-nucleo64-board-mb1355-stmicroelectronics-2.pdf
Official Documentation : https://www.waveshare.com/wiki/Pico-ePaper-2.9
Stm32 Library on Github: https://github.com/soonuse/epd-library-stm32/blob/master/2.9inch_e-paper/stm32/readme.md

## Nucleo Pins
SPI1_NSS - PA4/PB10 - D10
SPI1_MOSI - PA7 - D11
SPI1_MISO - PA6 - D12
SPI1_SCK - PA5 - D13 

WICHTIG: SCK muss PA5 sein (D13 auf dem Arduino-Header) wenn nicht: im konfigurator, ctrl+click und spi1_sck verschieben

## Wiring
| 2.9inch_e-paper -----> Nucleo Board (STM32WB55-RG)          | Comment                                                      |
|--------------------------------------------------------------|--------------------------------------------------------------|
| VCC ---------------> 3V3                                     | Power input                                                  |
| GND ---------------> GND                                     | Ground                                                       |
| DIN ---------------> SPI1_MOSI - PA7 - D11                  | MOSI pin of SPI interface, data transmitted from Host to Slave |
| CLK ---------------> SPI1_SCK  - PA5 - D13                  | SCK pin of SPI interface, clock input of the Slave           |
| CS  ---------------> SPI1_NSS (als GPIO Output) - PA4 - D10 | Chip select pin of SPI interface, Low Active                 |
| DC  ---------------> PC6 - D2  (GPIO Output)                | Data/Command control pin (High: Data; Low: Command)          |
| RST ---------------> PA9 - D9  (GPIO Output)                | Reset pin, low active                                        |
| BUSY --------------> PA8 - D6  (GPIO Input)                 | Busy output pin                                              |

## In CubeMX: Configuration

SPI should have data size of 8 bits


Diese Pins müssen manuell konfiguriert werden (nicht über SPI-Peripheral):

| Pin | User Label | Richtung     |
|-----|------------|--------------|
| PA4 | EPD_CS     | GPIO_Output  |
| PC6 | EPD_DC     | GPIO_Output  |
| PA9 | EPD_RST    | GPIO_Output  |
| PA8 | EPD_BUSY   | GPIO_Input   |

PA7 (MOSI) und PA5 (SCK) werden automatisch von SPI1 übernommen – die tauchen dann gelb im Pinout auf. Wenn nicht, ctrl+click to move pin.

GPIO Output Einstellungen für CS, DC, RST:
- Output Level: High
- Mode: Output Push Pull
- Pull: No pull
- Speed: High

## epdif.h
#include "stm32wbxx_hal.h"

#define EPD_CS_PIN      GPIO_PIN_4
#define EPD_CS_PORT     GPIOA

#define EPD_DC_PIN      GPIO_PIN_6
#define EPD_DC_PORT     GPIOC

#define EPD_RST_PIN     GPIO_PIN_9
#define EPD_RST_PORT    GPIOA

#define EPD_BUSY_PIN    GPIO_PIN_8
#define EPD_BUSY_PORT   GPIOA