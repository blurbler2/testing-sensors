/**
 * @file bme280.c
 * @brief BME280 sensor driver (I2C) implementing Bosch datasheet algorithms.
 *
 * This module provides initialization, calibration readout and integer
 * compensation routines for temperature, pressure and humidity. Public API is
 * declared in `bme280.h`.
 */

#include "bme280.h"
#include <string.h>

// Internal calibration structure
typedef struct {
  uint16_t dig_T1;
  int16_t  dig_T2;
  int16_t  dig_T3;
  uint16_t dig_P1;
  int16_t  dig_P2;
  int16_t  dig_P3;
  int16_t  dig_P4;
  int16_t  dig_P5;
  int16_t  dig_P6;
  int16_t  dig_P7;
  int16_t  dig_P8;
  int16_t  dig_P9;
  uint8_t  dig_H1;
  int16_t  dig_H2;
  uint8_t  dig_H3;
  int16_t  dig_H4;
  int16_t  dig_H5;
  int8_t   dig_H6;
  int32_t  t_fine;
} BME280_Calib_t;

static BME280_Calib_t calib;

/**
 * @brief Read bytes from a BME280 register over I2C.
 *
 * Performs a register write (register address) followed by a read of
 * `len` bytes into `buf` using the provided HAL I2C handle.
 *
 * @param hi2c Pointer to the I2C HAL handle.
 * @param addr 7-bit I2C address shifted left (HAL format).
 * @param reg Register address to start reading from.
 * @param buf Destination buffer for read data.
 * @param len Number of bytes to read.
 * @return HAL_StatusTypeDef HAL_OK on success, HAL_ERROR otherwise.
 */
static HAL_StatusTypeDef read_reg(I2C_HandleTypeDef *hi2c, uint16_t addr, uint8_t reg, uint8_t *buf, uint16_t len)
{
  if (HAL_I2C_Master_Transmit(hi2c, addr, &reg, 1, 100) != HAL_OK) return HAL_ERROR;
  return HAL_I2C_Master_Receive(hi2c, addr, buf, len, 200);
}

/**
 * @brief Write a single byte to a BME280 register over I2C.
 *
 * @param hi2c Pointer to the I2C HAL handle.
 * @param addr 7-bit I2C address shifted left (HAL format).
 * @param reg Register address to write.
 * @param val Value to write.
 * @return HAL_StatusTypeDef HAL_OK on success, HAL_ERROR otherwise.
 */
static HAL_StatusTypeDef write_reg(I2C_HandleTypeDef *hi2c, uint16_t addr, uint8_t reg, uint8_t val)
{
  uint8_t t[2] = {reg, val};
  return HAL_I2C_Master_Transmit(hi2c, addr, t, 2, 200);
}

/**
 * @brief Read calibration parameters from the sensor EEPROM.
 *
 * Reads the main coefficient block at 0x88..0xA1 and the humidity block
 * at 0xE1..0xE7 then decodes them into the `calib` structure.
 *
 * @param hi2c Pointer to the I2C HAL handle.
 * @param addr HAL-format I2C address.
 * @return HAL_StatusTypeDef HAL_OK on success, HAL_ERROR otherwise.
 */
static HAL_StatusTypeDef read_calib(I2C_HandleTypeDef *hi2c, uint16_t addr)
{
  uint8_t buf_main[26];
  uint8_t buf_hum[7];
  if (read_reg(hi2c, addr, 0x88, buf_main, 26) != HAL_OK) return HAL_ERROR;

  calib.dig_T1 = (buf_main[1] << 8) | buf_main[0];
  calib.dig_T2 = (int16_t)((buf_main[3] << 8) | buf_main[2]);
  calib.dig_T3 = (int16_t)((buf_main[5] << 8) | buf_main[4]);

  calib.dig_P1 = (buf_main[7] << 8) | buf_main[6];
  calib.dig_P2 = (int16_t)((buf_main[9] << 8) | buf_main[8]);
  calib.dig_P3 = (int16_t)((buf_main[11] << 8) | buf_main[10]);
  calib.dig_P4 = (int16_t)((buf_main[13] << 8) | buf_main[12]);
  calib.dig_P5 = (int16_t)((buf_main[15] << 8) | buf_main[14]);
  calib.dig_P6 = (int16_t)((buf_main[17] << 8) | buf_main[16]);
  calib.dig_P7 = (int16_t)((buf_main[19] << 8) | buf_main[18]);
  calib.dig_P8 = (int16_t)((buf_main[21] << 8) | buf_main[20]);
  calib.dig_P9 = (int16_t)((buf_main[23] << 8) | buf_main[22]);
  calib.dig_H1 = buf_main[25];

  if (read_reg(hi2c, addr, 0xE1, buf_hum, 7) != HAL_OK) return HAL_ERROR;
  calib.dig_H2 = (int16_t)((buf_hum[1] << 8) | buf_hum[0]);
  calib.dig_H3 = buf_hum[2];
  calib.dig_H4 = (int16_t)((buf_hum[3] << 4) | (buf_hum[4] & 0x0F));
  calib.dig_H5 = (int16_t)((buf_hum[5] << 4) | ((buf_hum[4] >> 4) & 0x0F));
  calib.dig_H6 = (int8_t)buf_hum[6];

  return HAL_OK;
}

/**
 * @brief Compute intermediate temperature compensation value `t_fine`.
 *
 * This implements the integer temperature compensation algorithm from
 * the BME280 datasheet. The function updates the module-global `calib.t_fine`.
 *
 * @param adc_T Raw 20-bit ADC temperature reading.
 */
static void comp_T(int32_t adc_T)
{
  int32_t var1, var2;
  var1 = ((((adc_T >> 3) - ((int32_t)calib.dig_T1 << 1))) * ((int32_t)calib.dig_T2)) >> 11;
  var2 = (((((adc_T >> 4) - ((int32_t)calib.dig_T1)) * ((adc_T >> 4) - ((int32_t)calib.dig_T1))) >> 12) * ((int32_t)calib.dig_T3)) >> 14;
  calib.t_fine = var1 + var2;
}

/**
 * @brief Convert `t_fine` to a temperature value in 0.01°C units.
 *
 * @return int32_t Temperature in hundredths of degrees Celsius.
 */
static int32_t comp_T_out(void)
{
  return (calib.t_fine * 5 + 128) >> 8; // 0.01 degC
}

/**
 * @brief Compensate raw pressure ADC reading to Pascal.
 *
 * Uses the integer algorithm from the datasheet and the previously computed
 * `t_fine` value. Returns pressure in Pa scaled by 256 (see datasheet
 * algorithm result shifting); the caller should interpret accordingly.
 *
 * @param adc_P Raw 20-bit ADC pressure reading.
 * @return uint32_t Compensated pressure value (Pa).
 */
static uint32_t comp_P_out(int32_t adc_P)
{
  int64_t var1, var2, p;
  var1 = ((int64_t)calib.t_fine) - 128000;
  var2 = var1 * var1 * (int64_t)calib.dig_P6;
  var2 = var2 + ((var1 * (int64_t)calib.dig_P5) << 17);
  var2 = var2 + (((int64_t)calib.dig_P4) << 35);
  var1 = ((var1 * var1 * (int64_t)calib.dig_P3) >> 8) + ((var1 * (int64_t)calib.dig_P2) << 12);
  var1 = (((int64_t)1 << 47) + var1) * ((int64_t)calib.dig_P1) >> 33;
  if (var1 == 0) return 0;
  p = 1048576 - adc_P;
  p = (((p << 31) - var2) * 3125) / var1;
  var1 = (((int64_t)calib.dig_P9) * (p >> 13) * (p >> 13)) >> 25;
  var2 = (((int64_t)calib.dig_P8) * p) >> 19;
  p = ((p + var1 + var2) >> 8) + (((int64_t)calib.dig_P7) << 4);
  return (uint32_t)p;
}

/**
 * @brief Compensate raw humidity ADC reading.
 *
 * Returns humidity in sensor-specific units (see datasheet). The caller
 * typically scales the returned value to %RH as needed.
 *
 * @param adc_H Raw 16-bit ADC humidity reading.
 * @return uint32_t Compensated humidity value (scaled internal units).
 */
static uint32_t comp_H_out(int32_t adc_H)
{
  int32_t v_x1_u32r;
  v_x1_u32r = (calib.t_fine - ((int32_t)76800));
  v_x1_u32r = (((((adc_H << 14) - (((int32_t)calib.dig_H4) << 20) - (((int32_t)calib.dig_H5) * v_x1_u32r)) + ((int32_t)16384)) >> 15) * (((((((v_x1_u32r * (int32_t)calib.dig_H6) >> 10) * (((v_x1_u32r * (int32_t)calib.dig_H3) >> 11) + ((int32_t)32768))) >> 10) + ((int32_t)2097152)) * (int32_t)calib.dig_H2 + 8192) >> 14));
  v_x1_u32r = v_x1_u32r - (((((v_x1_u32r >> 15) * (v_x1_u32r >> 15)) >> 7) * (int32_t)calib.dig_H1) >> 4);
  if (v_x1_u32r < 0) v_x1_u32r = 0;
  if (v_x1_u32r > 419430400) v_x1_u32r = 419430400;
  return (uint32_t)(v_x1_u32r >> 12);
}

/**
 * @brief Initialize the BME280 sensor and load calibration data.
 *
 * Performs a soft reset, reads calibration parameters, and programs
 * recommended control registers for standard operation.
 *
 * @param hi2c Pointer to the I2C HAL handle.
 * @param addr HAL-format I2C address of the device.
 * @return HAL_StatusTypeDef HAL_OK on success, HAL_ERROR otherwise.
 */
HAL_StatusTypeDef bme280_init(I2C_HandleTypeDef *hi2c, uint16_t addr)
{
  // soft reset
  if (write_reg(hi2c, addr, 0xE0, 0xB6) != HAL_OK) return HAL_ERROR;
  HAL_Delay(2);
  if (read_calib(hi2c, addr) != HAL_OK) return HAL_ERROR;
  // ctrl_hum x1
  if (write_reg(hi2c, addr, 0xF2, 0x01) != HAL_OK) return HAL_ERROR;
  // ctrl_meas temp x1 press x1 normal
  if (write_reg(hi2c, addr, 0xF4, 0x27) != HAL_OK) return HAL_ERROR;
  // config
  if (write_reg(hi2c, addr, 0xF5, 0xA0) != HAL_OK) return HAL_ERROR;
  HAL_Delay(500);
  return HAL_OK;
}

/**
 * @brief Read sensor registers and return compensated temperature, pressure, and humidity.
 *
 * This convenience function reads the raw ADC registers from 0xF7..0xFE,
 * runs the compensation algorithms and returns:
 *  - `temperature` in degrees Celsius (float)
 *  - `pressure` in Pascal (uint32_t)
 *  - `humidity` in %RH (float)
 *
 * @param hi2c Pointer to the I2C HAL handle.
 * @param addr HAL-format I2C address of the device.
 * @param temperature Pointer to float to receive °C temperature.
 * @param pressure Pointer to uint32_t to receive pressure in Pa.
 * @param humidity Pointer to float to receive %RH.
 * @return HAL_StatusTypeDef HAL_OK on success, HAL_ERROR otherwise.
 */
HAL_StatusTypeDef bme280_read_compensated(I2C_HandleTypeDef *hi2c, uint16_t addr,
                                          float *temperature, uint32_t *pressure, float *humidity)
{
  uint8_t buf[8];
  int32_t adc_T, adc_P, adc_H;
  if (read_reg(hi2c, addr, 0xF7, buf, 8) != HAL_OK) return HAL_ERROR;
  adc_P = ((int32_t)buf[0] << 12) | ((int32_t)buf[1] << 4) | ((int32_t)buf[2] >> 4);
  adc_T = ((int32_t)buf[3] << 12) | ((int32_t)buf[4] << 4) | ((int32_t)buf[5] >> 4);
  adc_H = ((int32_t)buf[6] << 8) | buf[7];

  comp_T(adc_T);
  *temperature = (float)comp_T_out() / 100.0f;
  *pressure = comp_P_out(adc_P);
  *humidity = (float)comp_H_out(adc_H) / 1024.0f; // comp_H_out returns x/1024 per some formulas
  return HAL_OK;
}
