# Hardware Architecture

- MCU: STM32WB55RG (P-NUCLEO-WB55). Dual-core family; project uses CM4 core.
- Peripherals used:
  - I2C1 on PB8 (SCL, D15) and PB9 (SDA, D14) for BME280, MPU-6050, and VEML7700.
  - SPI1 on PA5 (SCK, D13), PA7 (MOSI, D11), PA4 (CS, D10), PA6 (DC, D12) for ePaper display.
  - GPIOs for LEDs, display control (RST, BUSY), and MPU-6050 interrupt.

- **Sensor Connections:**

## BME280 (Temperature, Humidity, Pressure)
```
BME280        NUCLEO-WB55RG
--------------------------------
  - VCC -> 3V3
  - GND -> GND
  - SDA -> PB9 (D14)
  - SCL -> PB8 (D15)
  - CSB -> VCC (select I2C mode)
  - SDO -> GND (select address 0x76)
  - I2C Address: 0x76
  - 4.7k pull-ups on SDA and SCL to 3.3V
```

## MPU-6050 (6-Axis Accelerometer/Gyroscope)
```
MPU-6050      NUCLEO-WB55RG
--------------------------------
  - VCC -> 3V3
  - GND -> GND
  - XDA (SDA) -> PB9 (D14)  [Shared I2C1 with BME280 and VEML7700]
  - XCL (SCL) -> PB8 (D15)  [Shared I2C1]
  - ADO -> GND              [Sets I2C address to 0x68]
  - INT -> PH1 (D34)        [GPIO Input - Interrupt pin] // FEHLT NOCH?
  - I2C Address: 0x68
```

## VEML7700 (Ambient Light Sensor)
```
VEML7700      NUCLEO-WB55RG
--------------------------------
  - VCC -> 3V3
  - GND -> GND
  - SDA -> PB9 (D14)  [Shared I2C1]
  - SCL -> PB8 (D15)  [Shared I2C1]
  - I2C Address: 0x10 (default)
```

## Pico ePaper 2.9" Display (296×128 pixels)
```
Display Pin   Meaning         NUCLEO-WB55RG    Function
---------------------------------------------------------------
  - VCC -> 3V3
  - GND -> GND
  - DIN -> PA7 (D11)          [SPI1_MOSI]
  - CLK -> PA5 (D13)          [SPI1_SCK]
  - CS -> PA4 (D10)           [GPIO Output - Chip Select]
  - DC -> PC6 (D2)           [GPIO Output - High: Data/ Low: Command]
  - RST -> PA9 (D9)          [GPIO Output - Reset]
  - BUSY -> PA8 (D6)         [GPIO Input - Status Flag]
```

## I2C Bus Configuration (I2C1)
- **Pins:** PB8 (SCL), PB9 (SDA)
- **Devices:** BME280 (0x76), MPU-6050 (0x68), VEML7700 (0x10)
- **Pull-ups:** 4.7kΩ on both lines to 3.3V (already on breakout boards)

## SPI Bus Configuration (SPI1)
- **SCK:** PA5 (D13)
- **MOSI:** PA7 (D11)
- **CS:** PA4 (D10) - Software controlled (pulled HIGH = inactive)

## GPIO Assignments
```
Pin       Port    Function         Direction
----------------------------------------------
PA4       GPIO    SPI1 Chip Select (CS)  Output
PA5       GPIO    SPI1 SCK               (SPI peripheral)
PC6       GPIO    Display DC             Output
PA7       GPIO    SPI1 MOSI              (SPI peripheral)
PA8       GPIO    Display BUSY           Input
PA9       GPIO    Display RST            Output
PB8       GPIO    I2C1 SCL               (I2C peripheral)
PB9       GPIO    I2C1 SDA               (I2C peripheral)
PH1       GPIO    MPU-6050 INT           Input (with pull-down) FEHLT NOCH???
```

## Notes
- All sensors use **3.3V logic levels**. Do NOT connect 5V signals.
- I2C1 is shared by 3 devices with different addresses → no conflicts.
- Display BUSY pin must be checked before and after SPI writes.
- MPU-6050 INT pin can be used for motion detection or data-ready interrupts. FEHLT NOCH??? oder brauchen wir nicht???
- Short breadboard wires recommended for I2C to avoid timing issues.

