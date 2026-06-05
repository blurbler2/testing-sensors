#include "data_logger.h"
#include <stdio.h>

static FATFS fs;
static FIL   file;
static int   log_ready = 0;

int LOG_Init(void)
{
    if (f_mount(&fs, "", 1) != FR_OK)
        return -1;

    if (f_open(&file, "data.csv", FA_WRITE | FA_OPEN_ALWAYS) != FR_OK)
        return -2;

    if (f_size(&file) == 0)
    {
        f_printf(&file, "timestamp_ms,temp_C,pressure_Pa,humidity_pct,tilt_deg,lux\r\n");
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

void LOG_Sample(BME280_Data_t *bme, float tilt, VEML7700_Data_t *veml)
{
    if (!log_ready) return;

    static uint32_t sample_count = 0;
    uint32_t now = HAL_GetTick();

    int temp_i = (int)(bme->temperature * 10);
    int hum_i  = (int)(bme->humidity * 10);
    int tilt_i = (int)(tilt * 10);
    int lux_i  = (int)(veml->lux);

    f_printf(&file, "%lu,%d.%d,%lu,%d.%d,%d.%d,%d\r\n",
             (unsigned long)now,
             temp_i / 10, (temp_i < 0 ? -temp_i : temp_i) % 10,
             (unsigned long)bme->pressure,
             hum_i / 10, hum_i % 10,
             tilt_i / 10, (tilt_i < 0 ? -tilt_i : tilt_i) % 10,
             lux_i);

    sample_count++;
    if ((sample_count % 10) == 0)
        f_sync(&file);
}
