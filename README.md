# BME280 
Testing the sensor following this tutorial: https://microcontrollerslab.com/bme280-stm32-nucleo-stm32cubeide-oled/


## HW-611 BMP280 3.3 Digital Barometric Pressure Altitude Sensor
### Pins
```
BME280        NUCLEO-WB55RG
--------------------------------
- VCC ------> 3V3
- GND ------> GND
- SCL ------> PB8 (i2c clock, add a 4.7k resistor)
- SDA ------> PB9 (i2c data, add a 4.7k resistor)
- CSB ------> VCC (chip select, to activate i2c)
- SDO ------> GND (sets i2c adress to 0x76)
```

### Breadboard wiring (practical notes)

For the Nucleo wiring I used the Arduino-style D pins on the header:
- SDA -> D14 (PB9)
- SCL -> D15 (PB8)

Required parts for a simple breadboard setup:
- BME280 sensor module
- 4 wires from Nucleo to sensor (VCC, GND, SDA, SCL)
- two 4.7 kΩ pull-up resistors (one on SDA, one on SCL)
- two extra jumper wires to set CSB and SDO pins on the sensor module:
    - connect CSB to VCC to select I2C mode
    - connect SDO to GND to force I2C address 0x76

Wiring steps:
1. Place the BME280 module on the breadboard.
2. Connect VCC to the Nucleo 3V3 pin and GND to GND.
3. Connect SDA to D14 (PB9) and SCL to D15 (PB8).
4. Add a 4.7k pull-up resistor between SDA and 3.3V, and another 4.7k between SCL and 3.3V.
5. Tie CSB to 3.3V and SDO to GND (these must not be left floating).

This wiring reliably sets the sensor to I2C address 0x76 and prevents I2C timeouts caused by missing pull-ups or swapped SDA/SCL lines.

# I2C

```
The 7-bit device address is 111011x. The 6 MSB bits are fixed. The last bit is changeable by SDO
value and can be changed during operation. Connecting SDO to GND results in slave address
1110110 (0x76); connection it to VDDIO results in slave address 1110111 (0x77), which is the same as BMP280’s I²C address. 

SDO pin cannot be left floating; if left floating, the I²C address will be undefined.

CSB must be connected to VDDIO to select I²C interface

-datasheet page 32/33
```

## CubeMX Configuration

1. Neues Projekt -> Board Selector -> STM32 NUCLEO-WB55RG auswählen
2. I2C aktivieren -> Connectivity -> I2C1 -> Mode: I2C
    CubeMX setzt dann automatisch:
    ```
    PB8 (D15) → I2C1_SCL 
    PB9 (D14) → I2C1_SDA
    ```
3. I2C Parameter Settings 
    -> I2C Speed mode: Fast Mode 
    -> I2C Clock Speed 400000

    Oder: 100000Hz, Rest default
4. GPIO Settings: SDA und SCL als open drain, default

# Nucleo-WB55RG

## VDDIO/VCC/3.3 V
Auf dem STM32 P-Nucleo-WB55 Development-Kit ist die I/O-Spannung (VDDIO) fest an die Hauptversorgungsspannung (VCC bzw. VDD) gekoppelt. Da das Board über den USB-Anschluss betrieben wird, beträgt diese Spannung standardmäßig

# CODE

in `main.c` private define setzen:
```
#define BME280_ADDR (0x76 << 1)
``` 
STM32 HAL verwendet die Adresse linksverschoben.

Also:
echte Adresse = 0x76
HAL-Adresse = 0xEC

# VSCode
1. Install [Arm GNU Toolchain](https://developer.arm.com/downloads/-/arm-gnu-toolchain-downloads)
`arm-none-eabi-gcc --version`
2. In CubeMX, generate code
    you should have a cmake folder with
    ```CMakeLists.txt
    Core/
    Drivers/
    STM32CubeMX.ioc```
3. Configure CMake
    ```CTRL+SHIFT+P
    -> CMake: Configure```
    choose `arm-none-eabi-gcc` as kit
4. Build
    ```
    CTRL+SHIFT+P
    -> CMake: Build
    ```
dann sollte es ein settings.json geben, sobald es das gibt gibt es ganz unten im vscode editor ein "Build" button



```
[main] Building folder: /Users/blurbler/test-bme280/bme280/build/Debug 
[build] Starting build
[driver] NOTE: You are building with preset Debug, but there are some overrides being applied from your VS Code settings.
[proc] Executing command: cube-cmake --build /Users/blurbler/test-bme280/bme280/build/Debug --
[build] ninja: no work to do.
[driver] Build completed: 00:00:00.056
[build] Build finished with exit code 0
```



create a launch.json file
cmd + shift + p -> Tasks -> Run Task

you want this output after building