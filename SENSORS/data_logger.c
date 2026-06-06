#include "data_logger.h"
#include "diskio.h"
#include <stdio.h>
#include <string.h>

static FATFS fs;
static FIL   file;
static int   log_ready = 0;

/* ---- compile-time epoch from __DATE__ / __TIME__ ---- */
static const char *months = "JanFebMarAprMayJunJulAugSepOctNovDec";

static int month_num(const char *m)
{
    const char *p = months;
    int i;
    for (i = 0; i < 12; i++, p += 3)
        if (p[0] == m[0] && p[1] == m[1] && p[2] == m[2])
            return i + 1;
    return 1;
}

static int is_leap(int y) { return (y % 4 == 0 && y % 100 != 0) || y % 400 == 0; }

/* Unix epoch for the moment this file was compiled */
static uint32_t compile_epoch(void)
{
    char date[] = __DATE__;
    char time[] = __TIME__;
    /* "Mmm dd yyyy" */
    int m = month_num(date);
    int d = (date[4] == ' ') ? (date[5] - '0') : ((date[4] - '0') * 10 + (date[5] - '0'));
    int y = (date[7] - '0') * 1000 + (date[8] - '0') * 100 + (date[9] - '0') * 10 + (date[10] - '0');
    int hh = (time[0] - '0') * 10 + (time[1] - '0');
    int mm = (time[3] - '0') * 10 + (time[4] - '0');
    int ss = (time[6] - '0') * 10 + (time[7] - '0');
    /* days in months for non-leap year */
    static const uint8_t dim[] = {31,28,31,30,31,30,31,31,30,31,30,31};
    uint32_t epoch = 0;
    int i;
    for (i = 1970; i < y; i++)
        epoch += is_leap(i) ? 366 : 365;
    for (i = 0; i < m - 1; i++)
        epoch += dim[i] + (i == 1 && is_leap(y) ? 1 : 0);
    epoch += d - 1;
    epoch = epoch * 86400 + hh * 3600 + mm * 60 + ss;
    return epoch;
}

/* seconds elapsed since program start */
static uint32_t base_epoch;

static void init_epoch(void) { base_epoch = compile_epoch(); }

/* format "YYYY-MM-DD HH:MM:SS" from Unix epoch */
static void fmt_epoch(char *buf, size_t sz, uint32_t epoch)
{
    static const uint8_t dim[] = {31,28,31,30,31,30,31,31,30,31,30,31};
    int y = 1970;
    uint32_t s;
    for (;;) {
        uint32_t days = is_leap(y) ? 366 : 365;
        uint32_t secs = days * 86400;
        if (epoch < secs) break;
        epoch -= secs;
        y++;
    }
    int leap = is_leap(y);
    int m;
    for (m = 0; m < 12; m++) {
        uint32_t secs = (dim[m] + (m == 1 && leap ? 1 : 0)) * 86400;
        if (epoch < secs) break;
        epoch -= secs;
    }
    int d  = epoch / 86400 + 1;
    s      = epoch % 86400;
    int hh = s / 3600;
    int mm = (s % 3600) / 60;
    int ss = s % 60;
    snprintf(buf, sz, "%04d-%02d-%02d %02d:%02d:%02d", y, m + 1, d, hh, mm, ss);
}

int LOG_Init(void)
{
    init_epoch();

    if (f_mount(&fs, "", 1) != FR_OK)
        return -1;

    if (f_open(&file, "data.csv", FA_WRITE | FA_OPEN_ALWAYS) != FR_OK)
        return -2;

    if (f_size(&file) == 0)
    {
        f_printf(&file, "# Started: %s %s\r\n", __DATE__, __TIME__);
        f_printf(&file, "datetime,temp_C,pressure_Pa,humidity_pct,tilt_deg,lux\r\n");
    }
    else
    {
        f_lseek(&file, f_size(&file));
    }

    log_ready = 1;
    return 0;
}

int LOG_IsReady(void)
{
    return log_ready;
}

/* unmount, re-init SD, remount, reopen file for append */
static int LOG_Reinit(void)
{
    f_mount(NULL, "", 0);
    memset(&fs, 0, sizeof(fs));
    if (f_mount(&fs, "", 1) != FR_OK)
        return -1;
    if (f_open(&file, "data.csv", FA_WRITE | FA_OPEN_ALWAYS) != FR_OK)
        return -2;
    f_lseek(&file, f_size(&file));
    return 0;
}

void LOG_Sample(BME280_Data_t *bme, float tilt, VEML7700_Data_t *veml)
{
    /* if card was removed, attempt recovery every call */
    if (!log_ready)
    {
        if (LOG_Reinit() == 0)
            log_ready = 1;
        else
            return;
    }

    /* check physical presence via low-level driver */
    if (disk_status(0) & STA_NOINIT)
    {
        log_ready = 0;
        f_mount(NULL, "", 0);
        return;
    }

    static uint32_t sample_count = 0;
    uint32_t now = HAL_GetTick() / 1000;
    char dt[24];
    fmt_epoch(dt, sizeof(dt), base_epoch + now);

    int temp_i = (int)(bme->temperature * 10);
    int hum_i  = (int)(bme->humidity * 10);
    int tilt_i = (int)(tilt * 10);
    int lux_i  = (int)(veml->lux);

    if (f_printf(&file, "%s,%d.%d,%lu,%d.%d,%d.%d,%d\r\n",
             dt,
             temp_i / 10, (temp_i < 0 ? -temp_i : temp_i) % 10,
             (unsigned long)bme->pressure,
             hum_i / 10, hum_i % 10,
             tilt_i / 10, (tilt_i < 0 ? -tilt_i : tilt_i) % 10,
             lux_i) < 0)
    {
        log_ready = 0;
        f_mount(NULL, "", 0);
        return;
    }

    sample_count++;
    if ((sample_count % 10) == 0)
    {
        if (f_sync(&file) != FR_OK)
        {
            log_ready = 0;
            f_mount(NULL, "", 0);
        }
    }
}
