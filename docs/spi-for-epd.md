# SPI for E-Paper Display

## Pins (working configuration)

| Display | Nucleo | SPI Function |
|---------|--------|--------------|
| DIN     | PA7    | MOSI         |
| CLK     | PA5    | SCK          |
| CS      | PA4    | Chip Select  |
| DC      | PA2    | Data/Cmd     |
| RST     | PA1    | Reset        |
| BUSY    | PA3    | Busy (input) |

## Key Settings

- Data Size: 8 bits
- Clock Polarity: Low (CPOL=0)
- Clock Phase: 1 Edge (CPHA=0, SPI mode 0)
- Baud Rate Prescaler: `/16` (4 MHz from 64 MHz PCLK2)
- CS: software-controlled (GPIO)

## Notes

- Display writes use `EPD_2IN9_V2_Display_Base()` to send the same image to both RAM banks (0x24 and 0x26), preventing stale ghosting.
- The BUSY pin (PA3) must be configured as GPIO input with `GPIO_NOPULL`.
- SPI prescaler `/2` caused silent failures; `/16` is reliable.
