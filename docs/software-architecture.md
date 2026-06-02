# Software Architecture

```
Core/Src/main.c          — entry point: init, sensor read loop, display update
EPD/minimal_display.c    — 5-line e-paper text helper
EPD/EPD_2in9_V2.c        — Waveshare 2.9" V2 e-paper driver
EPD/Config/DEV_Config.c  — HAL SPI write and GPIO helpers
SENSORS/bme280.c         — BME280 driver (I2C, compensated reads)
SENSORS/mpu6050.c        — MPU-6050 driver (I2C)
SENSORS/veml7700.c       — VEML7700 driver (I2C, lux conversion)
```

- Build: CMake + `arm-none-eabi-gcc` toolchain
- Flash: ST-Link (OpenOCD or STM32CubeProgrammer)
- SPI prescaler: `/16` (4 MHz) — required for reliable e-paper communication
