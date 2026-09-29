/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "icache.h"
#include "tim.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

#include <stdio.h>

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
uint8_t last_switch_state = 2;
volatile uint32_t capture1 = 0;
volatile uint32_t capture2 = 0;
volatile uint32_t echo_time = 0;

volatile uint8_t first_capture = 1;
volatile uint8_t measurement_ready = 0;

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MPU_Config(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

void delay_us(uint16_t us)
{
    __HAL_TIM_SET_COUNTER(&htim1, 0);

    while (__HAL_TIM_GET_COUNTER(&htim1) < us)
    {
    }
}


void HCSR04_Trigger(void)
{
    /* Pregatim o masuratoare noua */
    measurement_ready = 0;
    first_capture = 1;

    __HAL_TIM_SET_CAPTUREPOLARITY(
        &htim1,
        TIM_CHANNEL_1,
        TIM_INPUTCHANNELPOLARITY_RISING
    );

    __HAL_TIM_CLEAR_FLAG(&htim1, TIM_FLAG_CC1);

    /* TRIG trebuie sa fie LOW initial */
    HAL_GPIO_WritePin(
        GPIOG,
        GPIO_PIN_12,
        GPIO_PIN_RESET
    );

    delay_us(2);

    /* Puls de trigger ~10 us */
    HAL_GPIO_WritePin(
        GPIOG,
        GPIO_PIN_12,
        GPIO_PIN_SET
    );

    delay_us(10);

    HAL_GPIO_WritePin(
        GPIOG,
        GPIO_PIN_12,
        GPIO_PIN_RESET
    );

    /*
     * Resetam counter-ul imediat dupa TRIG.
     * De aici TIM1 numara in microsecunde.
     */
    __HAL_TIM_SET_COUNTER(&htim1, 0);
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

  /* MPU Configuration--------------------------------------------------------*/
  MPU_Config();

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
  MX_ICACHE_Init();
  MX_TIM1_Init();
  MX_USART3_UART_Init();
  /* USER CODE BEGIN 2 */

    if (HAL_TIM_IC_Start_IT(&htim1, TIM_CHANNEL_1) != HAL_OK)
    {
        Error_Handler();
    }

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */

    while (1)
    {
        uint8_t switch_state =
            (HAL_GPIO_ReadPin(GPIOE, GPIO_PIN_11) == GPIO_PIN_SET);

        /* Afisam doar cand switch-ul isi schimba starea */
        if (switch_state != last_switch_state)
        {
            last_switch_state = switch_state;

            if (switch_state)
            {
                char message[] = "SENSOR ON\r\n";

                HAL_UART_Transmit(
                    &huart3,
                    (uint8_t *)message,
                    sizeof(message) - 1,
                    HAL_MAX_DELAY
                );
            }
            else
            {
                char message[] = "SENSOR OFF\r\n";

                HAL_UART_Transmit(
                    &huart3,
                    (uint8_t *)message,
                    sizeof(message) - 1,
                    HAL_MAX_DELAY
                );
            }
        }

        if (switch_state)
        {
            HCSR04_Trigger();

            HAL_Delay(100);

            if (measurement_ready)
            {
                measurement_ready = 0;

                uint32_t distance_cm = echo_time / 58;

                char buffer[64];

                int len = snprintf(
                    buffer,
                    sizeof(buffer),
                    "Distance: %lu cm\r\n",
                    (unsigned long)distance_cm
                );

                HAL_UART_Transmit(
                    &huart3,
                    (uint8_t *)buffer,
                    (uint16_t)len,
                    HAL_MAX_DELAY
                );
            }

            HAL_Delay(100);
        }
        else
        {
            HAL_Delay(100);
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
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE3);

  while(!__HAL_PWR_GET_FLAG(PWR_FLAG_VOSRDY)) {}

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSIDiv = RCC_HSI_DIV2;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2
                              |RCC_CLOCKTYPE_PCLK3;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_HSI;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB3CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_1) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure the programming delay
  */
  __HAL_FLASH_SET_PROGRAM_DELAY(FLASH_PROGRAMMING_DELAY_0);
}

/* USER CODE BEGIN 4 */

void HAL_TIM_IC_CaptureCallback(
    TIM_HandleTypeDef *htim
)
{
    if (
        htim->Instance == TIM1 &&
        htim->Channel == HAL_TIM_ACTIVE_CHANNEL_1
    )
    {
        /*
         * Prima intrerupere:
         * rising edge al semnalului ECHO.
         */
        if (first_capture)
        {
            capture1 =
                HAL_TIM_ReadCapturedValue(
                    htim,
                    TIM_CHANNEL_1
                );

            first_capture = 0;

            /*
             * Acum vrem sa prindem falling edge.
             */
            __HAL_TIM_SET_CAPTUREPOLARITY(
                htim,
                TIM_CHANNEL_1,
                TIM_INPUTCHANNELPOLARITY_FALLING
            );
        }

        /*
         * A doua intrerupere:
         * falling edge al semnalului ECHO.
         */
        else
        {
            capture2 =
                HAL_TIM_ReadCapturedValue(
                    htim,
                    TIM_CHANNEL_1
                );

            /*
             * Calculam latimea pulsului ECHO.
             * Tratam si cazul in care timerul
             * a facut overflow.
             */
            if (capture2 >= capture1)
            {
                echo_time =
                    capture2 - capture1;
            }
            else
            {
                echo_time =
                    (65536U - capture1)
                    + capture2;
            }

            measurement_ready = 1;
            first_capture = 1;

            /*
             * Pregatim timerul pentru
             * urmatoarea masuratoare.
             */
            __HAL_TIM_SET_CAPTUREPOLARITY(
                htim,
                TIM_CHANNEL_1,
                TIM_INPUTCHANNELPOLARITY_RISING
            );
        }
    }
}

/* USER CODE END 4 */

 /* MPU Configuration */

void MPU_Config(void)
{
  MPU_Region_InitTypeDef MPU_InitStruct = {0};
  MPU_Attributes_InitTypeDef MPU_AttributesInit = {0};

  /* Disables the MPU */
  HAL_MPU_Disable();

  /** Initializes and configures the Region 0 and the memory to be protected
  */
  MPU_InitStruct.Enable = MPU_REGION_ENABLE;
  MPU_InitStruct.Number = MPU_REGION_NUMBER0;
  MPU_InitStruct.BaseAddress = 0x08FFF000;
  MPU_InitStruct.LimitAddress = 0x08FFFFFF;
  MPU_InitStruct.AttributesIndex = MPU_ATTRIBUTES_NUMBER0;
  MPU_InitStruct.AccessPermission = MPU_REGION_ALL_RO;
  MPU_InitStruct.DisableExec = MPU_INSTRUCTION_ACCESS_DISABLE;
  MPU_InitStruct.IsShareable = MPU_ACCESS_NOT_SHAREABLE;

  HAL_MPU_ConfigRegion(&MPU_InitStruct);

  /** Initializes and configures the Attribute 0 and the memory to be protected
  */
  MPU_AttributesInit.Number = MPU_ATTRIBUTES_NUMBER0;
  MPU_AttributesInit.Attributes = INNER_OUTER(MPU_NOT_CACHEABLE);

  HAL_MPU_ConfigMemoryAttributes(&MPU_AttributesInit);
  /* Enables the MPU */
  HAL_MPU_Enable(MPU_PRIVILEGED_DEFAULT);

}

/**
  * @brief  This function is executed in case of error occurrence.
  * @param None
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */

    __disable_irq();

    while (1)
    {
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

  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
