# Testing Sensors on STM32WB55RG + 2.9" E-Paper Display

<img src="DOCS/working-demo.jpeg" width="50%" alt="Working demo">

Reads BME280 (T/P/H), MPU-6050 (accel), and VEML7700 (lux) sensors over I2C1 and displays readings on a Waveshare 2.9" V2 e-paper display (SPI1), updating every 2 seconds.

## Quick Start

```bash
cd build
cmake .. -G "Unix Makefiles" -DCMAKE_TOOLCHAIN_FILE=../cmake/arm-none-eabi-gcc.cmake
make -j$(nproc)
# Flash build/bme280.elf with ST-Link/OpenOCD
```

## Pin Connections

| Display   | Nucleo |
|-----------|--------|
| VCC       | 3V3    |
| GND       | GND    |
| DIN/MOSI  | PA7    |
| CLK/SCK   | PA5    |
| CS        | PA4    |
| DC        | PA2    |
| RST       | PA1    |
| BUSY      | PA3    |

Sensors share I2C1 (PB8=SCL, PB9=SDA). All use 3.3V logic, 4.7kΩ pull-ups on SDA/SCL.

## Display Layout

```
Testing sensors....
T:24.5C H:45.2%
P:101325 Pa
ACC 12 -4 256
LUX 320.0
```

## Key Files

- `Core/Src/main.c` — main loop, sensor reads, display update
- `EPD/minimal_display.c` — 5-line display helper using `EPD_2IN9_V2_Display_Base`
- `EPD/EPD_2in9_V2.c` — Waveshare 2.9" V2 e-paper driver
- `SENSORS/` — BME280, MPU-6050, VEML7700 drivers
