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
- SDD ------> GND (sets i2c adress to 0x76)
```

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
    PB8 → I2C1_SCL
    PB9 → I2C1_SDA
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