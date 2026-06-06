/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief           : Main program body - sensor data on e-paper display
  ******************************************************************************
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "bme280.h"
#include "mpu6050.h"
#include "veml7700.h"
#include "minimal_display.h"
#include "DEV_Config.h"
#include "EPD_2in9_V2.h"
#include "GUI_Paint.h"
#include "data_logger.h"
#include "app_fatfs.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
/* runtime-configurable intervals (ms); will be settable via BLE */
static uint32_t meas_interval  = 10000;  /* sensor read + SD log */
static uint32_t disp_interval  = 10000;  /* e-paper refresh (temp: match meas) */
#define BME280_ADDR      (0x76 << 1)
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

COM_InitTypeDef BspCOMInit;
I2C_HandleTypeDef hi2c1;

SPI_HandleTypeDef hspi1;

/* USER CODE BEGIN PV */
BME280_Data_t bme280_data = {0};
MPU6050_Data_t mpu6050_data = {0};
VEML7700_Data_t veml7700_data = {0};
int bme280_ready = 0;
int mpu6050_ready = 0;
int veml7700_ready = 0;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
void PeriphCommonClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_I2C1_Init(void);
static void MX_SPI1_Init(void);
/* USER CODE BEGIN PFP */
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* Configure the peripherals common clocks */
  PeriphCommonClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_I2C1_Init();
  MX_SPI1_Init();
  /* USER CODE BEGIN 2 */

  HAL_GPIO_WritePin(SPI1_EPD_CS_GPIO_Port, SPI1_EPD_CS_Pin, GPIO_PIN_SET);
  HAL_GPIO_WritePin(SPI1_SD_CS_GPIO_Port, SPI1_SD_CS_Pin, GPIO_PIN_SET);

  BSP_LED_Init(LED_RED);   /* error indicator only */

  BspCOMInit.BaudRate   = 115200;
  BspCOMInit.WordLength = COM_WORDLENGTH_8B;
  BspCOMInit.StopBits   = COM_STOPBITS_1;
  BspCOMInit.Parity     = COM_PARITY_NONE;
  BspCOMInit.HwFlowCtl  = COM_HWCONTROL_NONE;
  if (BSP_COM_Init(COM1, &BspCOMInit) != BSP_ERROR_NONE)
  {
    Error_Handler();
  }

  printf("\r\n=== Tree Sentinel ===\r\n");

  /* ---- FatFS init ---- */
  if (MX_FATFS_Init() != APP_OK) {
    printf("FATFS_Init FAILED\r\n");
    Error_Handler();
  }
  printf("FATFS driver linked OK\r\n");

  /* ---- SD / data logger ---- */
  if (LOG_Init() == 0) {
    printf("SD card + CSV logger ready\r\n");
  } else {
    printf("No SD card — logging disabled\r\n");
  }

  /* ---- Sensor init ---- */
  int bme280_ok = (bme280_init(&hi2c1, BME280_ADDR) == HAL_OK);
  int mpu6050_ok = (MPU6050_Init(&hi2c1) == HAL_OK);
  int veml7700_ok = (VEML7700_Init(&hi2c1) == HAL_OK);
  printf("Sensors: BME280=%d MPU6050=%d VEML7700=%d\r\n",
         bme280_ok, mpu6050_ok, veml7700_ok);

  /* ---- EPD init ---- */
  DEV_Module_Init();
  DEV_Delay_ms(1000);
  printf("EPD ready\r\n");

  /* ---- Tilt: calibrate reference at startup (board upright) ---- */
  MPU6050_Data_t mpu_data;
  float tilt_ref[3] = {0, 0, 0}, tilt_deg = 0.0f;
  if (mpu6050_ok) {
    MPU6050_ReadData(&hi2c1, &mpu_data);
    tilt_ref[0] = (float)mpu_data.ax / 16384.0f;
    tilt_ref[1] = (float)mpu_data.ay / 16384.0f;
    tilt_ref[2] = (float)mpu_data.az / 16384.0f;
  }

  uint32_t last_meas = 0, last_disp = 0;
  float temp = 0, hum = 0, lux = 0;
  uint32_t pres = 0;
  VEML7700_Data_t vdata = {0};

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    uint32_t now = HAL_GetTick();

    /* ---- read sensors + log SD (every meas_interval) ---- */
    if (now - last_meas >= meas_interval)
    {
      last_meas = now;

      /* BME280 */
      if (bme280_ok) {
        bme280_read_compensated(&hi2c1, BME280_ADDR, &temp, &pres, &hum);
      }

      /* MPU-6050 tilt */
      if (mpu6050_ok) {
        MPU6050_ReadData(&hi2c1, &mpu_data);
        float ax = (float)mpu_data.ax / 16384.0f;
        float ay = (float)mpu_data.ay / 16384.0f;
        float az = (float)mpu_data.az / 16384.0f;
        float dot = ax*tilt_ref[0] + ay*tilt_ref[1] + az*tilt_ref[2];
        float na = sqrtf(ax*ax + ay*ay + az*az);
        float nr = sqrtf(tilt_ref[0]*tilt_ref[0]
                       + tilt_ref[1]*tilt_ref[1]
                       + tilt_ref[2]*tilt_ref[2]);
        float cos_a = dot / (na * nr);
        if (cos_a > 1.0f) cos_a = 1.0f;
        if (cos_a < -1.0f) cos_a = -1.0f;
        tilt_deg = acosf(cos_a) * 57.29578f;
      }

      /* VEML7700 */
      if (veml7700_ok) {
        VEML7700_ReadALS(&hi2c1, &vdata);
        lux = vdata.lux;
      }

      /* restore SPI pins to AF mode (EPD bit-bangs them) */
      DEV_SPI_Init();

      /* Log to SD */
      BME280_Data_t bme = { .temperature = temp,
                            .pressure = pres,
                            .humidity = hum };
      LOG_Sample(&bme, tilt_deg, &vdata);
    }

    /* ---- refresh e-paper (every disp_interval) ---- */
    if (now - last_disp >= disp_interval)
    {
      last_disp = now;

      char l1[32], l2[32], l3[32], l4[32], l5[32];
      snprintf(l1, sizeof(l1), "Tree Sentinel");
      snprintf(l2, sizeof(l2), "T=%.1fC  P=%luPa", (double)temp, (unsigned long)pres);
      snprintf(l3, sizeof(l3), "H=%.0f%%  L=%.0flx", (double)hum, (double)lux);
      snprintf(l4, sizeof(l4), "Tilt=%.1fdeg", (double)tilt_deg);
      snprintf(l5, sizeof(l5), "SD:%s", LOG_IsReady() ? "OK" : "NO");
      Display_SensorData(l1, l2, l3, l4, l5);
    }

    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
  */
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI|RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = RCC_PLLM_DIV2;
  RCC_OscInitStruct.PLL.PLLN = 8;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLR = RCC_PLLR_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = RCC_PLLQ_DIV2;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure the SYSCLKSource, HCLK, PCLK1 and PCLK2 clocks dividers
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK4|RCC_CLOCKTYPE_HCLK2
                              |RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.AHBCLK2Divider = RCC_SYSCLK_DIV2;
  RCC_ClkInitStruct.AHBCLK4Divider = RCC_SYSCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_3) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief Peripherals Common Clock Configuration
  * @retval None
  */
void PeriphCommonClock_Config(void)
{
  RCC_PeriphCLKInitTypeDef PeriphClkInitStruct = {0};

  /** Initializes the peripherals clock
  */
  PeriphClkInitStruct.PeriphClockSelection = RCC_PERIPHCLK_SMPS;
  PeriphClkInitStruct.SmpsClockSelection = RCC_SMPSCLKSOURCE_HSI;
  PeriphClkInitStruct.SmpsDivSelection = RCC_SMPSCLKDIV_RANGE0;

  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInitStruct) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN Smps */

  /* USER CODE END Smps */
}

/**
  * @brief I2C1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_I2C1_Init(void)
{

  /* USER CODE BEGIN I2C1_Init 0 */

  /* USER CODE END I2C1_Init 0 */

  /* USER CODE BEGIN I2C1_Init 1 */

  /* USER CODE END I2C1_Init 1 */
  hi2c1.Instance = I2C1;
  hi2c1.Init.Timing = 0x00C12166;
  hi2c1.Init.OwnAddress1 = 0;
  hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c1.Init.OwnAddress2 = 0;
  hi2c1.Init.OwnAddress2Masks = I2C_OA2_NOMASK;
  hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
  if (HAL_I2C_Init(&hi2c1) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Analogue filter
  */
  if (HAL_I2CEx_ConfigAnalogFilter(&hi2c1, I2C_ANALOGFILTER_ENABLE) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Digital filter
  */
  if (HAL_I2CEx_ConfigDigitalFilter(&hi2c1, 0) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN I2C1_Init 2 */

  /* USER CODE END I2C1_Init 2 */

}

/**
  * @brief SPI1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_SPI1_Init(void)
{

  /* USER CODE BEGIN SPI1_Init 0 */

  /* USER CODE END SPI1_Init 0 */

  /* USER CODE BEGIN SPI1_Init 1 */

  /* USER CODE END SPI1_Init 1 */
  /* SPI1 parameter configuration*/
  hspi1.Instance = SPI1;
  hspi1.Init.Mode = SPI_MODE_MASTER;
  hspi1.Init.Direction = SPI_DIRECTION_2LINES;
  hspi1.Init.DataSize = SPI_DATASIZE_8BIT;
  hspi1.Init.CLKPolarity = SPI_POLARITY_LOW;
  hspi1.Init.CLKPhase = SPI_PHASE_1EDGE;
  hspi1.Init.NSS = SPI_NSS_SOFT;
  hspi1.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_256;
  hspi1.Init.FirstBit = SPI_FIRSTBIT_MSB;
  hspi1.Init.TIMode = SPI_TIMODE_DISABLE;
  hspi1.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
  hspi1.Init.CRCPolynomial = 7;
  hspi1.Init.CRCLength = SPI_CRC_LENGTH_DATASIZE;
  hspi1.Init.NSSPMode = SPI_NSS_PULSE_ENABLE;
  if (HAL_SPI_Init(&hspi1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN SPI1_Init 2 */
  /* SD card needs ≤ 400 kHz for init, 4 MHz for data */
  hspi1.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_16;
  HAL_SPI_Init(&hspi1);
  /* USER CODE END SPI1_Init 2 */

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  /* USER CODE BEGIN MX_GPIO_Init_1 */

  /* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_0|SPI1_RST_Pin|SPI1_DC_Pin|SPI1_EPD_CS_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(SPI1_SD_CS_GPIO_Port, SPI1_SD_CS_Pin, GPIO_PIN_SET);

  /*Configure GPIO pins : PA0 SPI1_RST_Pin SPI1_DC_Pin SPI1_EPD_CS_Pin */
  GPIO_InitStruct.Pin = GPIO_PIN_0|SPI1_RST_Pin|SPI1_DC_Pin|SPI1_EPD_CS_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pin : SPI1_BUSY_Pin */
  GPIO_InitStruct.Pin = SPI1_BUSY_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(SPI1_BUSY_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : SPI1_SD_CS_Pin */
  GPIO_InitStruct.Pin = SPI1_SD_CS_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(SPI1_SD_CS_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : USB_DM_Pin USB_DP_Pin */
  GPIO_InitStruct.Pin = USB_DM_Pin|USB_DP_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  GPIO_InitStruct.Alternate = GPIO_AF10_USB;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /* USER CODE BEGIN MX_GPIO_Init_2 */
  /* MPU-6050 INT (PH1) */
  __HAL_RCC_GPIOH_CLK_ENABLE();
  GPIO_InitStruct.Pin = GPIO_PIN_1;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLDOWN;
  HAL_GPIO_Init(GPIOH, &GPIO_InitStruct);
  /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
    BSP_LED_Toggle(LED_RED);
    HAL_Delay(100);
  }
  /* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
