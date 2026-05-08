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
#include <string.h>

#include "gesture.h"
#include "gesture_data.h"

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
#define MPU6050_GYRO_CONFIG 0x1B
#define MPU6050_ACC_CONFIG 0x1C
#define MPU6050_ACC_READ 0x3B
#define MPU6050_GYRO_READ 0x43
#define MPU6050_GYRO_SELF_TEST 0x0D
#define CALIBRATION_SAMPLES 1000

#define ACC_MAX 2
#define ACC_MIN -2
#define GYRO_MAX 250
#define GYRO_MIN -250

#define DATA_ROWS 40
#define DATA_COLS 6
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

static ai_handle gesture = AI_HANDLE_NULL;
AI_ALIGNED(32)
static ai_u8 activations [AI_GESTURE_DATA_ACTIVATIONS_SIZE];
AI_ALIGNED(32)
static ai_float in_data[1][DATA_ROWS][DATA_COLS];
AI_ALIGNED(32)
static ai_float out_data [AI_GESTURE_OUT_1_SIZE];
static ai_buffer *ai_input;
static ai_buffer *ai_output ;

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
float MinMaxScaler_Acc(float val);
float MinMaxScaler_Gyro(float val);
void Shift_Data(float data[1][DATA_ROWS][DATA_COLS], float newData[DATA_COLS]);
void Display_Data(float data[DATA_ROWS][DATA_COLS]);
int AI_Init(void);
int AI_Run(const void *in_data, void *out_data);
void Gather_Data(void);
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
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
  Gather_Data();
  if(AI_Init() != 0)
  {
	  printf("Failed to initialize AI model.\n");
	  while(1);
  }

  int counter = 0;
  printf("get ready!\n");
  HAL_Delay(5000);
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
	  if(counter%10==0){
		  uint32_t start_time = HAL_GetTick();
		  if(AI_Run(in_data, out_data) != 0)
		  {
			  printf("Inference failed.\n");
		  }
		  uint32_t end_time = HAL_GetTick();
		  printf("Inference time: %lu ms\n", end_time-start_time);
		  float *prediction = out_data;
		  float max = prediction[0];
		  int idx = 0;
		  for(int i = 0; i < 4; i++){
			  if(prediction[i] > max)
			  {
				  idx = i;
				  max = prediction[i];
			  }
		  }
		  printf("%d: %d\n", counter/10, idx);
	  }
	  MPU6050_ReadAcc();
	  MPU6050_ReadGyro();
	  float new_data[DATA_COLS] =
	  {
			  (ai_float)MinMaxScaler_Acc(acc_x),
			  (ai_float)MinMaxScaler_Acc(acc_y),
			  (ai_float)MinMaxScaler_Acc(acc_z),
			  (ai_float)MinMaxScaler_Gyro(gyro_x),
			  (ai_float)MinMaxScaler_Gyro(gyro_y),
			  (ai_float)MinMaxScaler_Gyro(gyro_z)

	  };
	  Shift_Data(in_data, new_data);
//	  printf("%f,%f,%f,%f,%f,%f\n", acc_x, acc_y, acc_z, gyro_x, gyro_y, gyro_z);
	  HAL_Delay(25);
	  counter++;
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
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLM = 8;
  RCC_OscInitStruct.PLL.PLLN = 180;
  RCC_OscInitStruct.PLL.PLLP = 2;
  RCC_OscInitStruct.PLL.PLLQ = 2;
  RCC_OscInitStruct.PLL.PLLR = 2;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Activate the Over-Drive mode
  */
  if (HAL_PWREx_EnableOverDrive() != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5) != HAL_OK)
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
	     * ACC_CONFIG: Sets full scale range, triggers self test.
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
	    HAL_I2C_Mem_Write(&hi2c1, MPU6050_ADDR, MPU6050_ACC_CONFIG, 1, &Data, 1, 1000);
	    Data = 0x0;
	    HAL_I2C_Mem_Write(&hi2c1, MPU6050_ADDR, MPU6050_GYRO_CONFIG, 1, &Data, 1, 1000);
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
	HAL_I2C_Mem_Read(&hi2c1, MPU6050_ADDR, MPU6050_ACC_READ, 1, rec_data, 6, 1000);

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

	HAL_I2C_Mem_Read (&hi2c1, MPU6050_ADDR, MPU6050_GYRO_READ, 1, rec_data, 6, 1000);

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

float MinMaxScaler_Acc(float val)
{
return (val - ACC_MIN) / (ACC_MAX - ACC_MIN);
}

float MinMaxScaler_Gyro(float val)
{
return (val - GYRO_MIN) / (GYRO_MAX - GYRO_MIN);
}

/*Shifting data to get a new frame for the ai model.*/
void Shift_Data(float data[1][DATA_ROWS][DATA_COLS], float newData[DATA_COLS])
{
	for (int i = DATA_ROWS - 1; i > 0; i--)
	{
		memcpy(data[0][i], data[0][i - 1], DATA_COLS * sizeof(float));
	}
	memcpy(data[0][0], newData, DATA_COLS * sizeof(float));
}

void Display_Data(float data[DATA_ROWS][DATA_COLS])
{
	for (int i = 0; i < DATA_ROWS; i++)
	{
		for (int j = 0; j < DATA_COLS; j++)
		{
			printf("%f ", data[i][j]);
		}
	}
}

/*AI model functions from documentation.*/
int AI_Init(void)
    {
    ai_error err;

    /* Create and initialize the c-model */
    const ai_handle acts[] =
	{
	activations
	};
    err = ai_gesture_create_and_init(&gesture, acts, NULL);
    if (err.type != AI_ERROR_NONE)
	{
	printf("aiInit failed! type=%d code=%d\n", err.type, err.code);
	return -1;
	}

    /* Reteive pointers to the model's input/output tensors */
    ai_input = ai_gesture_inputs_get(gesture, NULL);
    ai_output = ai_gesture_outputs_get(gesture, NULL);

    return 0;
    }

int AI_Run(const void *in_data, void *out_data)
    {
    if (gesture == AI_HANDLE_NULL)
	{
	printf("Network not initialized!\n");
	return -1;
	}
    ai_i32 n_batch;
    ai_error err;

    /* 1 - Update IO handlers with the data payload */
    ai_input[0].data = AI_HANDLE_PTR(in_data);
    ai_output[0].data = AI_HANDLE_PTR(out_data);

    /* 2 - Perform the inference */
    n_batch = ai_gesture_run(gesture, &ai_input[0], &ai_output[0]);
    if (n_batch != 1)
	{
	err = ai_gesture_get_error(gesture);
	printf("AI error: type=%d code=%d\n", err.type, err.code);
	return -1;
	};

    return 0;
    }

void Gather_Data()
{
	for(int i = 0; i < DATA_ROWS; i++){
		MPU6050_ReadAcc();
		MPU6050_ReadGyro();
		in_data[0][i][0] = (ai_float)MinMaxScaler_Acc(acc_x);
		in_data[0][i][1] = (ai_float)MinMaxScaler_Acc(acc_y);
		in_data[0][i][2] = (ai_float)MinMaxScaler_Acc(acc_z);
		in_data[0][i][3] = (ai_float)MinMaxScaler_Gyro(gyro_x);
		in_data[0][i][4] = (ai_float)MinMaxScaler_Gyro(gyro_y);
		in_data[0][i][5] = (ai_float)MinMaxScaler_Gyro(gyro_z);
		HAL_Delay(25);
	}
	printf("Done gathering data.\n");
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
