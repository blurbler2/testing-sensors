#include "veml7700.h"

/* Helper function to read register (16-bit) */
static HAL_StatusTypeDef VEML7700_ReadReg(I2C_HandleTypeDef *hi2c, uint8_t reg, uint16_t *data)
{
  uint8_t buffer[2];
  HAL_StatusTypeDef status = HAL_I2C_Mem_Read(hi2c, VEML7700_ADDR, reg, 
                                               I2C_MEMADD_SIZE_8BIT, buffer, 2, 100);
  if (status == HAL_OK)
    *data = (uint16_t)((buffer[1] << 8) | buffer[0]);  // Little-endian
  return status;
}

/* Helper function to write register (16-bit) */
static HAL_StatusTypeDef VEML7700_WriteReg(I2C_HandleTypeDef *hi2c, uint8_t reg, uint16_t data)
{
  uint8_t buffer[2] = {(uint8_t)(data & 0xFF), (uint8_t)((data >> 8) & 0xFF)};  // Little-endian
  return HAL_I2C_Mem_Write(hi2c, VEML7700_ADDR, reg, I2C_MEMADD_SIZE_8BIT, buffer, 2, 100);
}

/**
 * Initialize VEML7700
 */
HAL_StatusTypeDef VEML7700_Init(I2C_HandleTypeDef *hi2c)
{
  HAL_StatusTypeDef status;
  uint16_t conf_reg;

  // Read current configuration
  status = VEML7700_ReadReg(hi2c, VEML7700_REG_CONF, &conf_reg);
  if (status != HAL_OK)
    return HAL_ERROR;

  // Configure:
  // ALS Gain = 1/8 (0x01 << 11)
  // Integration time = 100ms (0x00 << 6)
  // Persistence = 1 (0x00 << 4)
  // Interrupt disabled (0x00 << 1)
  // Power on (0x00 << 0)
  conf_reg = (0x01 << 11) | (0x00 << 6) | (0x00 << 4) | (0x00 << 1) | 0x00;

  status = VEML7700_WriteReg(hi2c, VEML7700_REG_CONF, conf_reg);
  if (status != HAL_OK)
    return HAL_ERROR;

  return HAL_OK;
}

/**
 * Read ambient light sensor value
 */
HAL_StatusTypeDef VEML7700_ReadALS(I2C_HandleTypeDef *hi2c, VEML7700_Data_t *data)
{
  HAL_StatusTypeDef status;

  // Read raw ALS value
  status = VEML7700_ReadReg(hi2c, VEML7700_REG_ALS, &data->raw_als);
  if (status != HAL_OK)
    return HAL_ERROR;

  // Calculate lux
  data->lux = VEML7700_CalculateLux(data->raw_als);

  return HAL_OK;
}

/**
 * Convert raw ALS value to lux
 * Formula: Lux = ALS_raw * 0.0036 (for gain 1/8, integration time 100ms)
 */
float VEML7700_CalculateLux(uint16_t raw_als)
{
  // Conversion factor for Gain 1/8, Integration Time 100ms
  // This is an approximation; refer to datasheet for exact calibration
  float resolution = 0.0036f;
  
  return (float)raw_als * resolution;
}
