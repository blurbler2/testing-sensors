#ifndef MPU6050_H
#define MPU6050_H

#include "stm32wbxx_hal.h"
#include <stdint.h>

/* MPU-6050 I2C Address */
#define MPU6050_ADDR (0x68 << 1)  // 7-bit address shifted for HAL

/* MPU-6050 Registers */
#define MPU6050_REG_WHOAMI          0x75
#define MPU6050_REG_PWR_MGMT_1      0x6B
#define MPU6050_REG_CONFIG          0x1A
#define MPU6050_REG_GYRO_CONFIG     0x1B
#define MPU6050_REG_ACCEL_CONFIG    0x1C
#define MPU6050_REG_INT_ENABLE      0x38
#define MPU6050_REG_ACCEL_XOUT_H    0x3B
#define MPU6050_REG_ACCEL_XOUT_L    0x3C
#define MPU6050_REG_ACCEL_YOUT_H    0x3D
#define MPU6050_REG_ACCEL_YOUT_L    0x3E
#define MPU6050_REG_ACCEL_ZOUT_H    0x3F
#define MPU6050_REG_ACCEL_ZOUT_L    0x40
#define MPU6050_REG_GYRO_XOUT_H     0x43
#define MPU6050_REG_GYRO_XOUT_L     0x44
#define MPU6050_REG_GYRO_YOUT_H     0x45
#define MPU6050_REG_GYRO_YOUT_L     0x46
#define MPU6050_REG_GYRO_ZOUT_H     0x47
#define MPU6050_REG_GYRO_ZOUT_L     0x48

#define MPU6050_WHOAMI              0x68

typedef struct {
  int16_t ax, ay, az;  // Accelerometer raw values
  int16_t gx, gy, gz;  // Gyroscope raw values
} MPU6050_Data_t;

typedef struct {
  float roll;   // Rotation around X axis (degrees)
  float pitch;  // Rotation around Y axis (degrees)
  float yaw;    // Rotation around Z axis (degrees)
} MPU6050_Angles_t;

/* Function Prototypes */
HAL_StatusTypeDef MPU6050_Init(I2C_HandleTypeDef *hi2c);
HAL_StatusTypeDef MPU6050_ReadData(I2C_HandleTypeDef *hi2c, MPU6050_Data_t *data);
void MPU6050_CalculateAngles(MPU6050_Data_t *data, MPU6050_Angles_t *angles);

#endif // MPU6050_H
