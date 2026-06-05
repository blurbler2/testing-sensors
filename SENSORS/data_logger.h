#ifndef DATA_LOGGER_H
#define DATA_LOGGER_H

#include "main.h"
#include "veml7700.h"
#include "ff.h"

int  LOG_Init(void);
int  LOG_IsReady(void);
void LOG_Sample(BME280_Data_t *bme, float tilt, VEML7700_Data_t *veml);

#endif
