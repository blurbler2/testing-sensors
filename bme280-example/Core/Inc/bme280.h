/**
 * @file bme280.h
 * @brief Public API for the BME280 sensor driver.
 *
 * This header exposes a minimal interface for initializing the sensor and
 * reading compensated temperature, pressure and humidity values. The driver
 * implements the calibration and compensation algorithms described in the
 * Bosch BME280 datasheet.
 */

#ifndef BME280_H
#define BME280_H

#include "stm32wbxx_hal.h"
#include <stdint.h>

/**
 * @brief Initialize BME280 at I2C 8-bit address (HAL style: address<<1).
 *
 * Performs a soft-reset and reads calibration data from the sensor.
 *
 * @param hi2c Pointer to the I2C HAL handle.
 * @param addr HAL-format 8-bit I2C address (e.g. `0x76 << 1`).
 * @return HAL_StatusTypeDef HAL_OK on success, HAL_ERROR on failure.
 */
HAL_StatusTypeDef bme280_init(I2C_HandleTypeDef *hi2c, uint16_t addr);

/**
 * @brief Read compensated temperature, pressure and humidity.
 *
 * Temperature is returned in degrees Celsius, pressure in Pascals and humidity
 * as relative humidity in percent.
 *
 * @param hi2c Pointer to the I2C HAL handle.
 * @param addr HAL-format 8-bit I2C address.
 * @param temperature Pointer to float to receive degrees Celsius.
 * @param pressure Pointer to uint32_t to receive pressure in Pa.
 * @param humidity Pointer to float to receive %RH.
 * @return HAL_StatusTypeDef HAL_OK on success, HAL_ERROR on failure.
 */
HAL_StatusTypeDef bme280_read_compensated(I2C_HandleTypeDef *hi2c, uint16_t addr,
                                         float *temperature, uint32_t *pressure, float *humidity);

#endif // BME280_H
