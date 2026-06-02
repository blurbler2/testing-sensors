# Tree Sentinel — Next Steps

**Current sensors tested (I2C1):** BME280 (T/P/H), MPU-6050 (accel/tilt), VEML7700 (lux)  
**Display:** Waveshare 2.9" V2 e-paper (SPI1)  
**MCU:** STM32WB55RG (NUCLEO-WB55RG)

## System Schematic

```
  ┌─────────────────────────────────────────────────────────┐
  │                                                         │
  │   2× AA Eneloop Pro ──(+ VDD)──[ STM32WB55RG (NUCLEO) ] │
  │      (GND) ────────────(GND)────┤                       │
  │                                 |                       │
  │  ┌──────────────────────────────┤                       │
  │  │  I2C1 (PB8=SCL, PB9=SDA)     │                       │
  │  │                              │                       │
  │  │  ┌──────────┐ ┌──────────┐  ┌┴───────┐               │
  │  │  │  BME280  │ │ MPU6050  │  │VEML7700│               │
  │  │  │  0x76    │ │  0x68    │  │ 0x10   │               │
  │  │  │ T/P/H    │ │  tilt    │  │  lux   │               │
  │  │  └──────────┘ └──────────┘  └────────┘               │
  │  │                                                      │
  │  ├── SPI1 (PA5=SCK, PA7=MOSI, PA6=MISO)  ───────────────┤
  │  │                                                      │
  │  │  ┌────────────────────┐  ┌──────────────────────┐    │
  │  │  │ Waveshare 2.9" V2  │  │ Adafruit µSD         │    │
  │  │  │ e-paper (SPI)      │  │ SPI                  │    │
  │  │  │                    │  │                      │    │
  │  │  │ CS=PA4   DC=PA2    │  │ CS=PA0 (TBD)         │    │
  │  │  │ RST=PA1  BUSY=PA3  │  │                      │    │
  │  │  └────────────────────┘  └──────────────────────┘    │
  │  │                                                      │
  │  ├── I2C1 or separate ── SHT40 (0x44) ──────────────────┤
  │  │                                                      │
  │  └── GPIO (INT) ── Button (BLE wake) ───────────────────┘
  │                                                         │
  └─────────────────────────────────────────────────────────┘
```

## Next Steps

### 1. SD Card Module (SPI, shared with e-paper)

- SD module CS on a free GPIO (e.g. PB0, PB1, PC0, PC1, PC2, PC3, PB10, PB11, PB12–PB15, etc.)
- Share SPI1 with the e-paper — both are SPI slaves with independent CS
- Pinout: SD\_CS / MOSI / MISO / SCK / VCC / GND
  - MOSI=PA7, SCK=PA5 already wired to e-paper
  - MISO=PA6 (SPI1\_MISO) — not currently used by e-paper (unidirectional), so it's free
  - Add SD\_CS (new GPIO)
- FatFS library for file I/O
- CSV format: `timestamp, sensor_id, value, unit, status`

### 2. Power Module

**Chosen approach:** 2× AA Eneloop Pro (NiMH), direct to Nucleo VDD, bypassing the on-board 5V regulator.

- Eneloop Pro: -20–50°C, ~2500 mAh, 1.2V nominal → 2.4V total
- STM32WB55 VDD range: 1.71–3.6V → plenty of headroom
- No buck-boost, no LDO needed — simplest, most reliable, cold-proof
- Replace or recharge when voltage drops below ~1.9V (plenty of margin above 1.71V brownout)
- If rechargeable convenience not required: CR123A primary (3V, -20–60°C) also works

**Not recommended for this project:**
- Standard Li-ion (freezes at 0°C)
- MC33063A (high Iq, bulky, poor efficiency at light load)
- Single-cell chemistries requiring buck-boost (adds complexity, efficiency loss, failure point)

### 3. Additional Sensors (Add-On PCB)

- **SHT31** (I2C, addr 0x44) — separate air T/H sensor (redundant to BME280 or use both)
- Button (GPIO interrupt wake from sleep, trigger BLE advertising)

### 4. KiCad Schematic

1. Start with STM32WB55RG minimal schematic (crystal, decoupling, SWD)
2. Add I2C1 bus with BME280, MPU-6050, VEML7700, SHT31
3. Add SPI1 bus with e-paper connector + SD card slot
4. Add button with debounce → GPIO
6. Add power section: battery connector, charger, LDO, protection
7. Add connector to Nucleo (or design as shield)

### 5. Firmware Architecture

```
main loop (2s interval):
  wake → read sensors → store to SD → update display → sleep

BLE:
  advertising on button press
  config: interval, time, thresholds
  data export via BLE

Low Power:
  RTC wake timer (STOP mode)
  e-paper: init → update → sleep
  sensors: power off when idle
```

### 6. Enclosure

- 3D-printed box mounted at trunk midpoint
- SD card accessible from outside
- Button on outside
- Cable gland for sensors

## Current Status

| Component          | Status |
|--------------------|--------|
| BME280 (T/P/H)     | Working |
| MPU-6050 (tilt)    | Working |
| VEML7700 (lux)     | Working |
| E-paper display    | Working |
| SD card module     | **Next** |
| Power module       | **Next** |
| SHT31              | Optional |
| Button             | TODO |
| BLE                | TODO |
| KiCad schematic    | TODO |
| PCB design         | TODO |
| Enclosure          | TODO |
