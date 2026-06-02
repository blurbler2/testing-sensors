#ifndef VEML7700_H
#define VEML7700_H

#include "stm32wbxx_hal.h"
#include <stdint.h>

/* VEML7700 I2C Address */
#define VEML7700_ADDR (0x10 << 1)  // 7-bit address shifted for HAL

/* VEML7700 Registers */
#define VEML7700_REG_CONF           0x00
#define VEML7700_REG_THRESD_HIGH    0x01
#define VEML7700_REG_THRESD_LOW     0x02
#define VEML7700_REG_POWER_SAVE     0x03
#define VEML7700_REG_ALS            0x04
#define VEML7700_REG_WHITE          0x05
#define VEML7700_REG_ALS_HIGH       0x06
#define VEML7700_REG_ALS_LOW        0x07

typedef struct {
  uint16_t raw_als;     // Raw ALS value
  float lux;            // Calculated lux value
} VEML7700_Data_t;

/* Function Prototypes */
HAL_StatusTypeDef VEML7700_Init(I2C_HandleTypeDef *hi2c);
HAL_StatusTypeDef VEML7700_ReadALS(I2C_HandleTypeDef *hi2c, VEML7700_Data_t *data);
float VEML7700_CalculateLux(uint16_t raw_als);

#endif // VEML7700_H
