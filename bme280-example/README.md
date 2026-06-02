# BME280 - Multi-Sensor Application

STM32WB55RG firmware reading BME280, MPU6050, VEML7700 and displaying on a Waveshare 2.9" Pico ePaper.

## Build & Flash

### VS Code Tasks (Cmd+Shift+B)

| Task | Shortcut | Command |
|------|----------|---------|
| **build** | Cmd+Shift+B | `cmake --build build/Debug` |
| **flash** | — | `openocd ... program ... verify reset exit` |
| **build & flash** | — | build then flash |

### Terminal

```sh
# Build
cmake --build build/Debug

# Flash
openocd -f interface/stlink.cfg -f target/stm32wbx.cfg -c "program ${workspaceFolder}/build/Debug/bme280.elf verify reset exit"
```

## Dependencies

- ARM GCC toolchain (`arm-none-eabi-gcc`)
- CMake + Ninja
- OpenOCD
- ST-Link (on-board)
