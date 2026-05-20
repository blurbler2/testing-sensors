# Hardware Architecture

- MCU: STM32WB55RG (P-NUCLEO-WB55). Dual-core family; project uses CM4 core.
- Peripherals used:
  - I2C1 on PB8 (SCL, D15) and PB9 (SDA, D14) for BME280.
  - UART (COM1) for printf/serial debug at 115200.
  - GPIOs for LEDs and optional SPI for e-paper.

- BME280 connections:
```
BME280        NUCLEO-WB55RG
--------------------------------
  - VCC -> 3V3
  - GND -> GND
  - SDA -> PB9 (D14)
  - SCL -> PB8 (D15)
  - CSB -> VCC (select I2C)
  - SDO -> GND (select address 0x76)
  - 4.7k pull-ups on SDA and SCL to 3.3V
````

- Notes:
  - Ensure CSB/SDO are not floating; SDO selects 0x76 (GND) or 0x77 (VCC).
  - Use level-compatible 3.3V signals; do NOT tie sensor to 5V.
  - On breadboard use short connections and proper pull-ups to avoid I2C TIMEOUT.
