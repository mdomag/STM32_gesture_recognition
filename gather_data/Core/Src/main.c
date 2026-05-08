/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2025 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "i2c.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "stdio.h"
#include "stdint.h"
#include "math.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define MPU6050_ADDR 0xD0
// MPU6050 registers.
#define MPU6050_WHO_AM_I 0x75
#define MPU6050_PWR_MGMT_1 0x6B
#define MPU6050_SMPRT_DIV 0x19
#define MPU6050_CONFIG 0x1A
#define MPU6050_gyro_CONFIG 0x1B
#define MPU6050_ACCEL_CONFIG 0x1C
#define MPU6050_ACCEL_READ 0x3B
#define MPU6050_gyro_READ 0x43
#define MPU6050_SELF_TEST 0x0D
#define CALIBRATION_SAMPLES 1000
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
int16_t acc_x_raw, acc_y_raw, acc_z_raw;
int16_t gyro_x_raw, gyro_y_raw, gyro_z_raw;

int16_t acc_x_offset = 0, acc_y_offset = 0, acc_z_offset = 0;
int16_t gyro_x_offset = 0, gyro_y_offset = 0, gyro_z_offset = 0;

float acc_x, acc_y, acc_z;
float gyro_x, gyro_y, gyro_z;

float acc_lsb = 16384.0;
float gyro_lsb = 131.0;

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */
void MPU6050_Init(void);
void MPU6050_Reset(void);
void MPU6050_Calibrate(void);
void MPU6050_ReadAcc(void);
void MPU6050_ReadGyro(void);
int ReadButton(void);


/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
// Redirecting printf to SWO, needs syscall.
int __io_putchar(int ch)
{
	ITM_SendChar(ch);
	return(ch);
}
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

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_USART2_UART_Init();
  MX_I2C1_Init();
  /* USER CODE BEGIN 2 */
  MPU6050_Init();
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  MPU6050_Calibrate();
  HAL_Delay(5000);
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
	  MPU6050_ReadAcc();
	  MPU6050_ReadGyro();
	  printf("%f, %f, %f, %f, %f, %f\n", acc_x, acc_y, acc_z, gyro_x, gyro_y, gyro_z);
	  HAL_Delay(25);
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
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE3);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLM = 16;
  RCC_OscInitStruct.PLL.PLLN = 336;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV4;
  RCC_OscInitStruct.PLL.PLLQ = 2;
  RCC_OscInitStruct.PLL.PLLR = 2;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */
void MPU6050_Init(void)
{
	HAL_StatusTypeDef status = HAL_I2C_IsDeviceReady(&hi2c1, MPU6050_ADDR, 1, 1000);
	if(status == HAL_OK)
	{
	    printf("I2C device is ready.\n");
	}
	else
	{
	    printf("Error connecting to the I2C device.\n");
	}
	uint8_t check;
	/*
	 * This register is used to verify the identity of the device.
	 * The default value of the register for MPU6050 is 0x68.
	 */
	HAL_I2C_Mem_Read (&hi2c1, MPU6050_ADDR, MPU6050_WHO_AM_I, 1, &check, 1, 1000);
	if (check == 0x68)
	{
	    printf("MPU6050 is ready!\n");
	    uint8_t Data;
	    /*
	     * PWR_MGMT_1: Wakes up MPU6050. Turns off temperature sensor.
	     * SMPRT_DIV: Specifies the divider from the gyroscope output rate used to generate the Sample Rate.
	     * Sample Rate = gyroscope Output Rate / (1 + SMPLRT_DIV). -> 8KHZ / (1 +7) = 1KHz
	     * ACCEL_CONFIG: Sets full scale range, triggers self test.
	     * Accelometer range set to: 2g, sensibility: 16384.
	     * gyro_CONFIG: Sets full scale range, triggers self test.
	     * gyroscope range set to: 250 °/s, sensibility 16384.
	     */
	    Data = 0b00001000;
	    HAL_I2C_Mem_Write(&hi2c1, MPU6050_ADDR, MPU6050_PWR_MGMT_1, 1, &Data, 1, 1000);
	    Data = 0x0;
	    HAL_I2C_Mem_Write(&hi2c1, MPU6050_ADDR, MPU6050_CONFIG, 1, &Data, 1, 1000);
	    Data = 0x10;
	    HAL_I2C_Mem_Write(&hi2c1, MPU6050_ADDR, MPU6050_SMPRT_DIV, 1, &Data, 1,  1000);
	    Data = 0x0;
	    HAL_I2C_Mem_Write(&hi2c1, MPU6050_ADDR, MPU6050_ACCEL_CONFIG, 1, &Data, 1, 1000);
	    Data = 0x0;
	    HAL_I2C_Mem_Write(&hi2c1, MPU6050_ADDR, MPU6050_gyro_CONFIG, 1, &Data, 1, 1000);
	    HAL_Delay(300);
	}
}

void MPU6050_Calibrate(void)
{
    int32_t acc_x_sum = 0;
    int32_t acc_y_sum = 0;
    int32_t acc_z_sum = 0;

    int32_t gyro_x_sum = 0;
    int32_t gyro_y_sum = 0;
    int32_t gyro_z_sum = 0;

    for (int i = 0; i < CALIBRATION_SAMPLES; i++)
    {
        // Read acc and gyro values.
    	MPU6050_ReadAcc();
        MPU6050_ReadGyro();

        // Sum the values;
        acc_x_sum += acc_x_raw;
        acc_y_sum += acc_y_raw;
        acc_z_sum += acc_z_raw;

        gyro_x_sum += gyro_x_raw;
        gyro_y_sum += gyro_y_raw;
        gyro_z_sum += gyro_z_raw;

        HAL_Delay(10);
    }

    // Get average.
    acc_x_offset = acc_x_sum / CALIBRATION_SAMPLES;
    acc_y_offset = acc_y_sum / CALIBRATION_SAMPLES;
    acc_z_offset = acc_z_sum / CALIBRATION_SAMPLES - (int16_t)acc_lsb;

    gyro_x_offset = gyro_x_sum / CALIBRATION_SAMPLES;
    gyro_y_offset = gyro_y_sum / CALIBRATION_SAMPLES;
    gyro_z_offset = gyro_z_sum / CALIBRATION_SAMPLES;

    printf("Calibration Complete!\n");
    printf("Accelerometer Offsets: X = %d, Y = %d, Z = %d\n", acc_x_offset, acc_y_offset, acc_z_offset);
    printf("gyroscope Offsets: X = %d, Y = %d, Z = %d\n", gyro_x_offset, gyro_y_offset, gyro_z_offset);
}

void MPU6050_Reset(void) {
    uint8_t reset_command = 0x80;
    HAL_I2C_Mem_Write(&hi2c1, MPU6050_ADDR, MPU6050_PWR_MGMT_1, 1, &reset_command, 1, 1000);
    HAL_Delay(100);
}

void MPU6050_ReadAcc(void)
{
	/*
	 * Read from 6 registers all the Accel values.
	 */
	uint8_t rec_data[6];
	HAL_I2C_Mem_Read(&hi2c1, MPU6050_ADDR, MPU6050_ACCEL_READ, 1, rec_data, 6, 1000);

	/*
	 * Raw value is 16 bit - merge every two registers.
	 */
	 acc_x_raw = (int16_t)(rec_data[0] << 8 | rec_data[1]) - acc_x_offset;
	 acc_y_raw = (int16_t)(rec_data[2] << 8 | rec_data[3]) - acc_y_offset;
	 acc_z_raw = (int16_t)(rec_data[4] << 8 | rec_data[5]) - acc_z_offset;

	/*
	 * Normalization: raw data to degrees per second.
	 * Analog data -> digital data.
	 * LSB Sensitivity defines physical value corresponds to one unit in the digital scale. (?)
	 * Divide by the set sensibility for chosen range.
	 */
	acc_x = (float)acc_x_raw/acc_lsb;
	acc_y = (float)acc_y_raw/acc_lsb;
	acc_z = (float)acc_z_raw/acc_lsb;
}

void MPU6050_ReadGyro(void)
{
	/*
  	 * Read from 6 registers all the gyro values.
  	 */
	uint8_t rec_data[6];

	HAL_I2C_Mem_Read (&hi2c1, MPU6050_ADDR, MPU6050_gyro_READ, 1, rec_data, 6, 1000);

	/*
	* Raw value is 16 bit - merge every two registers.
	*/
	gyro_x_raw = (int16_t)(rec_data[0] << 8 | rec_data[1]) - gyro_x_offset;
	gyro_y_raw = (int16_t)(rec_data[2] << 8 | rec_data[3]) - gyro_y_offset;
	gyro_z_raw = (int16_t)(rec_data[4] << 8 | rec_data[5]) - gyro_z_offset;;

	/*
	 * Normalization: raw data to degrees per second.
	 * Divide by the set sensibility for chosen range.
	 */
	gyro_x = (float)gyro_x_raw/gyro_lsb;
	gyro_y = (float)gyro_y_raw/gyro_lsb;
	gyro_z = (float)gyro_z_raw/gyro_lsb;
}

int ReadButton(void)
{
	int button_state = HAL_GPIO_ReadPin(B1_GPIO_Port, B1_Pin);
	return button_state;
}
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
  }
  /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
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
