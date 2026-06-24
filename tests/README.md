# Test-Konzept: HAL-Treiber für I²C, SPI, RTC

**Framework:** Unity + CMock
**Umgebung:** Host-basiert (x86) mit automatisch generierten Mocks
**Plattform:** STM32WB55RG (Cortex-M4, 64 MHz)

## 1. Architektur

```
tests/
├── CMakeLists.txt            # Übersetzt die Tests für die Host-Plattform
├── unity/                    # Unity-Framework (subtree oder Source-Copy)
├── mocks/
│   ├── MockI2C.c / .h        # CMock-generiert aus stm32wbxx_hal_i2c.h
│   ├── MockSPI.c / .h        # CMock-generiert aus stm32wbxx_hal_spi.h
│   └── MockRTC.c / .h        # CMock-generiert aus stm32wbxx_hal_rtc.h
├── test_sensors/
│   ├── test_bme280.c
│   ├── test_mpu6050.c
│   └── test_veml7700.c
├── test_init_sequence.c
├── test_stop2_resume.c
└── test_rtc.c
```

Die Sensor-Quelldateien (`SENSORS/bme280.c`, `SENSORS/mpu6050.c`, `SENSORS/veml7700.c`) werden **unkomprimiert** mitübersetzt. Nur die HAL-Ebene wird durch Mocks ersetzt.

## 2. CMake-Integration

Im Haupt-`CMakeLists.txt`:

```cmake
option(BUILD_TESTS "Build unit tests" OFF)
if(BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
```

`tests/CMakeLists.txt`:
- Setzt den Host-Compiler (clang/gcc) ein
- Übersetzt `../SENSORS/*.c` mit `-DUNIT_TESTING` und `-DUSE_HAL_DRIVER`
- Linkt Unity + CMock-Mocks
- Registriert jede `test_*.c` als CTest-Testfall
- Exkludiert `Drivers/STM32WBxx_HAL_Driver/` – die HAL wird komplett gemockt

## 3. Testfälle pro Device

### BME280 (I²C, Adresse 0x76)

| # | Testfall | Beschreibung |
|---|----------|-------------|
| 1 | `test_bme280_init_sequence` | Prüft korrekte Reihenfolge: Soft-Reset → ctrl_hum → ctrl_meas → Kalibrier-lesen → config |
| 2 | `test_bme280_init_nack` | Sensor antwortet nicht → `bme280_init()` gibt `HAL_ERROR` zurück |
| 3 | `test_bme280_init_timeout` | I²C-Timeout → `bme280_init()` gibt `HAL_TIMEOUT` zurück |
| 4 | `test_bme280_read_temperature` | Bekannte Rohdaten + Kalibrierdaten → erwartete Temperatur in °C |
| 5 | `test_bme280_read_pressure` | Bekannte Rohdaten → erwarteter Druck in hPa |
| 6 | `test_bme280_read_humidity` | Bekannte Rohdaten → erwartete Feuchte in %rF |
| 7 | `test_bme280_null_handle` | `hi2c = NULL` → kein Crash, definierter Fehlerwert |

### MPU6050 (I²C, Adresse 0x68)

| # | Testfall | Beschreibung |
|---|----------|-------------|
| 1 | `test_mpu6050_init_sequence` | WHO_AM_I lesen → Power-Management konfigurieren → korrekte Registerreihenfolge |
| 2 | `test_mpu6050_who_am_i_wrong` | Falsche WHO_AM_I-Antwort → Init schlägt fehl |
| 3 | `test_mpu6050_read_accel` | Bekannte Rohdaten → skalierte Beschleunigung in g |
| 4 | `test_mpu6050_read_gyro` | Bekannte Rohdaten → skalierte Drehrate in °/s |
| 5 | `test_mpu6050_tilt_calculation` | Neigungswinkel aus Beschleunigungsdaten korrekt berechnet |

### VEML7700 (I²C, Adresse 0x10)

| # | Testfall | Beschreibung |
|---|----------|-------------|
| 1 | `test_veml7700_init_sequence` | ALS_CONF_0 schreiben → ALS_WH/ALS_WL setzen → korrekte Reihenfolge |
| 2 | `test_veml7700_read_als` | Bekannte Rohdaten → Lux-Wert korrekt berechnet |
| 3 | `test_veml7700_gain_switching` | Bei hoher Helligkeit wird Verstärkung umgeschaltet |

## 4. Init-Reihenfolge-Tests

Die korrekte Initialisierungsreihenfolge wird über ein Log-Array geprüft:

| Schritt | Funktion | Abhängigkeit |
|---------|----------|-------------|
| 1 | `HAL_Init()` | – |
| 2 | `SystemClock_Config()` | HAL |
| 3 | `MX_GPIO_Init()` | RCC |
| 4 | `MX_I2C1_Init()` | GPIO, RCC |
| 5 | `MX_SPI1_Init()` | GPIO, RCC |
| 6 | `MX_FATFS_Init()` | SPI |
| 7 | `bme280_init()` | I2C |
| 8 | `MPU6050_Init()` | I2C |
| 9 | `VEML7700_Init()` | I2C |
| 10 | `DEV_Module_Init()` | SPI, GPIO |

**Testfälle:**

| Testfall | Beschreibung |
|----------|-------------|
| `test_init_order_correct` | Komplette Sequenz durchlaufen → Reihenfolge-Array mit Soll-Reihenfolge vergleichen |
| `test_bme280_fails_without_i2c` | I2C-Init auslassen → `bme280_init()` soll definiert fehlschlagen |
| `test_fatfs_fails_without_spi` | SPI-Init auslassen → `MX_FATFS_Init()` soll definiert fehlschlagen |

## 5. STOP2-Resume-Tests

STOP2 setzt die Peripherie zurück. Nach dem Aufwachen müssen I²C, SPI und ggf. RTC neu initialisiert werden.

**Simulation:** `HAL_I2C_MspDeInit()` + `HAL_SPI_MspDeInit()` aufrufen (setzt Hardware-Register zurück), dann erneut `MX_I2C1_Init()` / `MX_SPI1_Init()`.

**Testfälle:**

| Testfall | Beschreibung |
|----------|-------------|
| `test_i2c_resume_after_stop2` | Init → DeInit → ReInit → Sensordaten lesen funktioniert |
| `test_spi_resume_after_stop2` | Init → DeInit → ReInit → SPI-Transaktion funktioniert |
| `test_rtc_persists_after_stop2` | RTC-Zeit setzen → STOP2 simulieren → Zeit läuft weiter (kein Reset auf 1.1.1970) |
| `test_all_peripherals_available_after_resume` | Komplette Sequenz: Init → STOP2 → Resume → alle 3 Sensoren lesbar + SPI-Transaktion möglich |

## 6. RTC-Tests

(Da RTC aktuell nicht im Build ist, muss `stm32wbxx_hal_rtc.c` zunächst in `cmake/stm32cubemx/CMakeLists.txt` aufgenommen werden.)

| Testfall | Beschreibung |
|----------|-------------|
| `test_rtc_set_and_get_time` | Zeit setzen → auslesen → stimmt überein |
| `test_rtc_date_rollover` | 23:59:59 → 1 Sekunde später → Datum korrekt erhöht |
| `test_rtc_alarm_triggers` | Alarm in 100ms → Flag wird gesetzt |

## 7. Aufwandsabschätzung

| Modul | Testfälle | Aufwand (Tage) |
|-------|-----------|:--------------:|
| BME280 | 7 | 1,0 |
| MPU6050 | 5 | 1,0 |
| VEML7700 | 3 | 0,5 |
| Init-Reihenfolge | 3 | 0,5 |
| STOP2-Resume | 4 | 1,0 |
| RTC | 3 | 0,5 |
| Test-Infrastruktur (CMake, CMock, Unity) | – | 1,0 |
| **Gesamt** | **~25 – 30** | **~5,5 Tage** |

## 8. Nächste Schritte

1. Unity als Git-Submodule oder Source-Copy in `tests/unity/` einbinden
2. CMock-Ruby-Setup (`gem install cmock`)
3. Erstes minimales Test-Target: `test_bme280_init_sequence` als Durchstich
4. Nach und nach alle Testfälle ergänzen
5. Init-Reihenfolge-Tests
6. STOP2-Resume-Tests
7. Abschluss: `ctest --output-on-failure` läuft grün durch
