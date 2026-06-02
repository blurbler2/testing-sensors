# Hardware Architecture

- MCU: STM32WB55RG (P-NUCLEO-WB55)
- Display: Waveshare 2.9" V2 e-paper (296×128, SPI1)
- Sensors: BME280, MPU-6050, VEML7700 (all on I2C1)

## Pin Mapping

| Signal     | Pin  | Function    |
|------------|------|-------------|
| EPD CS     | PA4  | GPIO Output |
| EPD DC     | PA2  | GPIO Output |
| EPD RST    | PA1  | GPIO Output |
| EPD BUSY   | PA3  | GPIO Input  |
| EPD MOSI   | PA7  | SPI1_MOSI   |
| EPD SCK    | PA5  | SPI1_SCK    |
| I2C1 SCL   | PB8  | I2C1_SCL    |
| I2C1 SDA   | PB9  | I2C1_SDA    |

All sensors at 3.3V logic. 4.7kΩ pull-ups on SDA/SCL. MPU-6050 ADO tied to GND (addr 0x68). BME280 CSB tied to VCC (I2C mode), SDO to GND (addr 0x76).
