# Tree Sentinel — Next Steps

**Current sensors tested (I2C1):** BME280 (T/P/H), MPU-6050 (accel/tilt), VEML7700 (lux)  
**Display:** Waveshare 2.9" V2 e-paper (SPI1, bit-banged)  
**SD card:** Adafruit MicroSD ADA4682 (SPI1, hardware)  
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
  │  │  │  (bit-banged GPIO) │  │  (hardware SPI)      │    │
  │  │  │                    │  │                      │    │
  │  │  │ CS=PA9   DC=PA2    │  │ CS=PA4               │    │
  │  │  │ RST=PA1  BUSY=PA3  │  │                      │    │
  │  │  └────────────────────┘  └──────────────────────┘    │
  │  │                                                      │
  │  ├── I2C1 ── SHT40 (0x44) ──────────────────────────────┤
  │  │                                                      │
  │  └── GPIO (INT) ── Button (BLE wake) ───────────────────┘
  │                                                         │
  └─────────────────────────────────────────────────────────┘
```

## Next Steps

### 1. SD Card — Flash Test ✓ (driver integrated, needs testing)

- FatFS + reference `user_diskio_spi.c` integrated (proven on same HW)
- CS=PA4 (`SPI1_SD_CS`), shared SPI1 with EPD
- **Known bug:** EPD bit-bangs PA5/PA7 to GPIO mode; `DEV_SPI_Init()` restores AF mode before each SD write
- **Known bug:** FatFS `f_printf` doesn't support `%f` — floats decomposed to `int.int` in `data_logger.c`
- **Known bug:** SD card file dates show 1970 — no RTC (`get_fattime` unimplemented)
- CSV format: `timestamp_ms,temp_C,pressure_Pa,humidity_pct,tilt_deg,lux`
- **To test:** `f_mount` + CSV log create → verify on PC
- **If SD fails:** bypass TXB0104 level shifter on ADA4682, wire 3.3V direct

### SD Card Hot-Plug Recovery (branch `feature/sd-hotplug`)

**What happens without recovery:** If the SD card is removed while logging, `HAL_SPI_TransmitReceive` timeouts silently, FatFS returns `FR_DISK_ERR`, and `f_printf`/`f_sync` fail. The logger sets `log_ready = 0` permanently — card reinsertion does nothing.

**Implemented recovery (on `feature/sd-hotplug`):**
- Every `LOG_Sample()` call checks `disk_status(0)` — if `STA_NOINIT`, marks card gone, unmounts stale volume
- On subsequent calls, attempts `LOG_Reinit()`: unmount → `memset(&fs, 0)` → remount (triggers `disk_initialize` SPI re-init) → reopen `data.csv` in append mode
- `f_printf()` and `f_sync()` failures also trigger detach + retry
- EPD shows "SD:OK" / "SD:--" based on `LOG_IsReady()`
- **Not merged** to `main` — still on `feature/sd-hotplug`, needs testing

### 2. Power Module ✓ (decided)

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

- **SHT40** (I2C, addr 0x44) — separate air T/H sensor (complements BME280)
- Button (GPIO interrupt wake from sleep, trigger BLE advertising)

### 4. KiCad Schematic

1. Start with STM32WB55RG minimal schematic (crystal, decoupling, SWD)
2. Add I2C1 bus with BME280, MPU-6050, VEML7700, SHT40
3. Add SPI1 bus with e-paper connector + SD card slot
4. Add button with debounce → GPIO
5. Add power section: battery connector
6. Add connector to Nucleo (or design as shield)

### 5. Firmware Roadmap

```
Current (implemented):
  main loop (2s interval):
    read sensors → update e-paper → log CSV to SD → repeat

Next:
  BLE:
    advertising on button press
    config: interval, time, thresholds
    data export via BLE

Later:
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

| Component             | Status              |
|-----------------------|---------------------|
| BME280 (T/P/H)        | Working             |
| MPU-6050 (tilt)       | Working             |
| VEML7700 (lux)        | Working             |
| E-paper display       | Working             |
| SD card module        | Integrated, needs flash test |
| Power module          | Decided (2× AA)     |
| SHT40                 | Optional            |
| KiCad schematic       | TODO                |
| BLE                   | TODO                |
| Button for wake       | TODO                |
| PCB design            | TODO                |
| Enclosure             | TODO                |

## Open questions
What happens when SD Card is removed while nucleo is working?