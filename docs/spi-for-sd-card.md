# SD Card via SPI — NUCLEO-WB55RG

Writes "Hello World!" to an SD card using FatFs over SPI.

## Wiring
```
| SD Card | NUCLEO-WB55RG |
|---------|---------------|
| CLK     | PA5 (D13)     |
| MOSI    | PA7 (D11)     |
| MISO    | PA6 (D12)     |
| CS      | PA4 (D10)     |
| VCC     | 3V3           |
| GND     | GND           |
```
DET is unwired.

## Build

```bash
cmake --preset Debug
cmake --build --preset Debug
```

Output: `build/Debug/test-sd-card.bin`

## Flash

```bash
st-flash --reset write build/Debug/test-sd-card.bin 0x08000000
```

or

```bash
openocd -f interface/stlink.cfg -f target/stm32wbx.cfg \
  -c "program build/Debug/test-sd-card.elf verify reset exit"
```

## Indication

- **Green LED** blinks 5× then stays on → write successful
- **Red LED** blinks 5× then stays on → write failed

## CubeMX Setup

**System Core (Pinout & Configuration)**
- RCC → enable HSE
- SYS → Debug → Serial Wire

**Clock Config**
- HSE, sysclock 64 MHz

**SPI**
- Enable SPI, 8 bit, prescaler 256 (highest baud rate divider)
- Move CLK to PA5 (Ctrl+Click)

**Pins**
- PA4 (renamed: `SPI1_CS` → GPIO_Output, Output level: High,  ), Pull-up

## References

- [STM32 Adafruit SD Shield driver](https://github.com/STMicroelectronics/STM32CubeF0/blob/master/Drivers/BSP/Adafruit_Shield/stm32_adafruit_sd.c)
- [FatFs sample by kiwih](https://github.com/kiwih/cubeide-sd-card/tree/master)
- [Tutorial video](https://www.youtube.com/watch?v=PBIm8BCnbyQ)

## Hardware used
- ADA4682 Adafruit Micro SD SPI oder SDIO Karten Breakout Board - 3V
- Nucleo Board STM32WB55RG: MB1355D
## Demo

<img src="docs/working-demo.jpeg" alt="Working demo" width="50%">