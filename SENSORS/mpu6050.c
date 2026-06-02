#include "mpu6050.h"
#include <math.h>

/* Helper function to read register */
static HAL_StatusTypeDef MPU6050_ReadReg(I2C_HandleTypeDef *hi2c, uint8_t reg, uint8_t *data)
{
  return HAL_I2C_Mem_Read(hi2c, MPU6050_ADDR, reg, I2C_MEMADD_SIZE_8BIT, data, 1, 100);
}

/* Helper function to write register */
static HAL_StatusTypeDef MPU6050_WriteReg(I2C_HandleTypeDef *hi2c, uint8_t reg, uint8_t data)
{
  return HAL_I2C_Mem_Write(hi2c, MPU6050_ADDR, reg, I2C_MEMADD_SIZE_8BIT, &data, 1, 100);
}

/**
 * Initialize MPU-6050
 */
HAL_StatusTypeDef MPU6050_Init(I2C_HandleTypeDef *hi2c)
{
  uint8_t whoami;
  HAL_StatusTypeDef status;

  // Read WHOAMI register to verify device
  status = MPU6050_ReadReg(hi2c, MPU6050_REG_WHOAMI, &whoami);
  if (status != HAL_OK || whoami != MPU6050_WHOAMI)
  {
    return HAL_ERROR;
  }

  // Power Management - Wake up device (clear sleep bit)
  status = MPU6050_WriteReg(hi2c, MPU6050_REG_PWR_MGMT_1, 0x00);
  if (status != HAL_OK) return HAL_ERROR;

  // Digital Low Pass Filter configuration
  // 0x06 = DLPF_CFG = 6 (5Hz cutoff)
  status = MPU6050_WriteReg(hi2c, MPU6050_REG_CONFIG, 0x06);
  if (status != HAL_OK) return HAL_ERROR;

  // Gyroscope configuration
  // 0x08 = FS_SEL = 1 (±500 deg/s)
  status = MPU6050_WriteReg(hi2c, MPU6050_REG_GYRO_CONFIG, 0x08);
  if (status != HAL_OK) return HAL_ERROR;

  // Accelerometer configuration
  // 0x00 = AFS_SEL = 0 (±2g)
  status = MPU6050_WriteReg(hi2c, MPU6050_REG_ACCEL_CONFIG, 0x00);
  if (status != HAL_OK) return HAL_ERROR;

  // Interrupt enable - Data ready interrupt
  status = MPU6050_WriteReg(hi2c, MPU6050_REG_INT_ENABLE, 0x01);
  if (status != HAL_OK) return HAL_ERROR;

  return HAL_OK;
}

/**
 * Read accelerometer and gyroscope data
 */
HAL_StatusTypeDef MPU6050_ReadData(I2C_HandleTypeDef *hi2c, MPU6050_Data_t *data)
{
  uint8_t buffer[14];
  HAL_StatusTypeDef status;

  // Read all data at once (14 bytes from ACCEL_XOUT_H)
  status = HAL_I2C_Mem_Read(hi2c, MPU6050_ADDR, MPU6050_REG_ACCEL_XOUT_H, 
                            I2C_MEMADD_SIZE_8BIT, buffer, 14, 100);
  
  if (status != HAL_OK)
    return HAL_ERROR;

  // Combine high and low bytes (MSB first, two's complement)
  data->ax = (int16_t)((buffer[0] << 8) | buffer[1]);
  data->ay = (int16_t)((buffer[2] << 8) | buffer[3]);
  data->az = (int16_t)((buffer[4] << 8) | buffer[5]);
  
  // Gyroscope data (buffer[6] and [7] are temperature)
  data->gx = (int16_t)((buffer[8] << 8) | buffer[9]);
  data->gy = (int16_t)((buffer[10] << 8) | buffer[11]);
  data->gz = (int16_t)((buffer[12] << 8) | buffer[13]);

  return HAL_OK;
}

/**
 * Calculate roll, pitch, yaw from accelerometer and gyroscope data
 * Simplified calculation using only accelerometer for static angles
 */
void MPU6050_CalculateAngles(MPU6050_Data_t *data, MPU6050_Angles_t *angles)
{
  // Convert raw accelerometer data to g (±2g range, LSB = 16384 counts/g)
  float ax_g = (float)data->ax / 16384.0f;
  float ay_g = (float)data->ay / 16384.0f;
  float az_g = (float)data->az / 16384.0f;

  // Calculate roll (rotation around X axis)
  angles->roll = atan2f(ay_g, az_g) * 180.0f / M_PI;

  // Calculate pitch (rotation around Y axis)
  angles->pitch = asinf(-ax_g) * 180.0f / M_PI;

  // Yaw cannot be determined from accelerometer alone (needs magnetometer or gyro integration)
  // For now, set yaw to 0 or could integrate gyroscope Z
  angles->yaw = 0.0f;
}
