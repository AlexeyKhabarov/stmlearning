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
 * This software is licensed under terms that can be found in the LICENSE file
 * in the root directory of this software component.
 * If no LICENSE file comes with this software, it is provided AS-IS.
 *
 ******************************************************************************
 */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
typedef enum
{
  LED_MODE_OFF = 0,
  LED_MODE_ON,
  LED_MODE_BLINK_SLOW,
  LED_MODE_BLINK_FAST,
} LedMode_t;

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
ADC_HandleTypeDef hadc1;
DMA_HandleTypeDef hdma_adc1;

TIM_HandleTypeDef htim2;

UART_HandleTypeDef huart6;
HAL_StatusTypeDef status;
/* USER CODE BEGIN PV */
static uint32_t button_last_tick = 0;
static uint32_t last_blink = 0;
volatile uint32_t pending_presses = 0;
volatile uint32_t press_count = 0;
static uint8_t blink_active = 0;
static uint8_t blink_toggles_left = 0;
static uint32_t blink_tick = 0;
static LedMode_t led_mode = LED_MODE_OFF;
static uint32_t led_mode_tick = 0;
volatile uint8_t rx_byte;
volatile uint8_t rx_buffer[64];
volatile uint8_t rx_index = 0;
volatile uint8_t command_ready;
static const char led_on[] = "LED ON\r\n";
static const char led_off[] = "LED OFF\r\n";
static const char led_blink_slow[] = "LED BLINK SLOW\r\n";
static const char led_blink_fast[] = "LED BLINK FAST\r\n";
volatile uint8_t tim_flag = 0;
volatile int32_t measured_adc1_ch10_code = 0;
static int16_t time_between_measurements = 0;
static int8_t continue_measuring = 0;
static uint32_t prev_measurement = 0;
static uint32_t adc_dma_value = 0;
volatile uint8_t adc_flag = 0;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_DMA_Init(void);
static void MX_USART6_UART_Init(void);
static void MX_TIM2_Init(void);
static void MX_ADC1_Init(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

static void led_set_on(void)
{
  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_1, GPIO_PIN_RESET);
}

static void led_set_off(void)
{
  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_1, GPIO_PIN_SET);
}
void parsing_measurement_command(char *str)
{
  char *first, *second;
  first = strtok(str, " ");
  second = strtok(NULL, " ");
  if (second != NULL)
  {
    time_between_measurements = atoi(second);
    if (time_between_measurements == 0)
    {
      continue_measuring = 0;
    }
    else
    {
      continue_measuring = 1;
      prev_measurement = HAL_GetTick();
    }
  }
}
void execute_command(const char *str)
{
  if (strcmp(str, "led on") == 0)
  {
    led_mode = LED_MODE_ON;
  }
  else if (strcmp(str, "led off") == 0)
  {
    led_mode = LED_MODE_OFF;
  }
  else if (strcmp(str, "status") == 0)
  {
    switch (led_mode)
    {
    case LED_MODE_OFF:
      HAL_UART_Transmit(&huart6, (uint8_t *)led_off, sizeof(led_off) - 1, HAL_MAX_DELAY);
      break;
    case LED_MODE_ON:
      HAL_UART_Transmit(&huart6, (uint8_t *)led_on, sizeof(led_on) - 1, HAL_MAX_DELAY);
      break;
    case LED_MODE_BLINK_SLOW:
      HAL_UART_Transmit(&huart6, (uint8_t *)led_blink_slow, sizeof(led_blink_slow) - 1, HAL_MAX_DELAY);
      break;
    case LED_MODE_BLINK_FAST:
      HAL_UART_Transmit(&huart6, (uint8_t *)led_blink_fast, sizeof(led_blink_fast) - 1, HAL_MAX_DELAY);
      break;
    default:
      HAL_UART_Transmit(&huart6, (uint8_t *)led_off, sizeof(led_off) - 1, HAL_MAX_DELAY);
      break;
    }
  }
  else if (strncmp(str, "measurement", 11) == 0)
  {
    if (strlen(str) == 11)
    {
      status = HAL_ADC_Start_DMA(&hadc1, &adc_dma_value, 1);
      if (status == HAL_BUSY)
      {
        static const char line_bisy[] = "ADC busy\r\n";
        HAL_UART_Transmit(&huart6, (uint8_t *)line_bisy, sizeof(line_bisy) - 1, HAL_MAX_DELAY);
      }
      else if (status == HAL_ERROR)
      {
        static const char adc_error[] = "ADC error\r\n";
        HAL_UART_Transmit(&huart6, (uint8_t *)adc_error, sizeof(adc_error) - 1, HAL_MAX_DELAY);
      }
    }

    else if (str[11] == ' ')
    {
      parsing_measurement_command(str);
    }
  }
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
  MX_DMA_Init();
  MX_USART6_UART_Init();
  MX_TIM2_Init();
  MX_ADC1_Init();
  /* USER CODE BEGIN 2 */
  led_set_off();
  const char msg[] = "UART6 is ready\r\n";
  HAL_UART_Transmit(&huart6, (uint8_t *)msg, sizeof(msg) - 1, HAL_MAX_DELAY);
  HAL_UART_Receive_IT(&huart6, &rx_byte, 1);
  HAL_TIM_Base_Start_IT(&htim2);
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
    uint32_t now = HAL_GetTick();

    if (tim_flag)
    {
      led_mode = (led_mode == LED_MODE_OFF) ? LED_MODE_ON : LED_MODE_OFF;
      tim_flag = 0;
    }

    if (command_ready)
    {
      execute_command((char *)rx_buffer);
      command_ready = 0;
      rx_index = 0;
      memset(rx_buffer, 0, sizeof(rx_buffer));
    }

    if (continue_measuring == 1)
    {
      if ((now - prev_measurement) >= time_between_measurements)
      {
        status = HAL_ADC_Start_DMA(&hadc1, &adc_dma_value, 1);
        if (status == HAL_BUSY)
        {
          static const char line_bisy[] = "ADC busy\r\n";
          HAL_UART_Transmit(&huart6, (uint8_t *)line_bisy, sizeof(line_bisy) - 1, HAL_MAX_DELAY);
        }
        else if (status == HAL_ERROR)
        {
          static const char adc_error[] = "ADC error\r\n";
          HAL_UART_Transmit(&huart6, (uint8_t *)adc_error, sizeof(adc_error) - 1, HAL_MAX_DELAY);
        }
        prev_measurement = now;
      }
    }

    if (adc_flag)
    {
      char buf[16];
      int len = sprintf(buf, "ADC: %lu\r\n", adc_dma_value);
      HAL_UART_Transmit(&huart6, (uint8_t *)buf, len, HAL_MAX_DELAY);
      adc_flag = 0;
    }

    if (pending_presses > 0U)
    {
      pending_presses--;
      led_mode++;
      if (led_mode > LED_MODE_BLINK_FAST)
      {
        led_mode = LED_MODE_OFF;
      }
      led_mode_tick = now;
    }
    switch (led_mode)
    {
    case LED_MODE_OFF:
      led_set_off();
      break;
    case LED_MODE_ON:
      led_set_on();
      break;
    case LED_MODE_BLINK_SLOW:
      if ((now - led_mode_tick) >= 500U)
      {
        HAL_GPIO_TogglePin(GPIOA, GPIO_PIN_1);
        led_mode_tick = now;
      }
      break;
    case LED_MODE_BLINK_FAST:
      if ((now - led_mode_tick) >= 150U)
      {
        HAL_GPIO_TogglePin(GPIOA, GPIO_PIN_1);
        led_mode_tick = now;
      }
      break;
    default:
      led_mode = LED_MODE_OFF;
      break;
    }
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
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 8;
  RCC_OscInitStruct.PLL.PLLN = 336;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 4;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
   */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
 * @brief ADC1 Initialization Function
 * @param None
 * @retval None
 */
static void MX_ADC1_Init(void)
{

  /* USER CODE BEGIN ADC1_Init 0 */

  /* USER CODE END ADC1_Init 0 */

  ADC_ChannelConfTypeDef sConfig = {0};

  /* USER CODE BEGIN ADC1_Init 1 */

  /* USER CODE END ADC1_Init 1 */

  /** Configure the global features of the ADC (Clock, Resolution, Data Alignment and number of conversion)
   */
  hadc1.Instance = ADC1;
  hadc1.Init.ClockPrescaler = ADC_CLOCK_SYNC_PCLK_DIV4;
  hadc1.Init.Resolution = ADC_RESOLUTION_12B;
  hadc1.Init.ScanConvMode = DISABLE;
  hadc1.Init.ContinuousConvMode = DISABLE;
  hadc1.Init.DiscontinuousConvMode = DISABLE;
  hadc1.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_NONE;
  hadc1.Init.ExternalTrigConv = ADC_SOFTWARE_START;
  hadc1.Init.DataAlign = ADC_DATAALIGN_RIGHT;
  hadc1.Init.NbrOfConversion = 1;
  hadc1.Init.DMAContinuousRequests = DISABLE;
  hadc1.Init.EOCSelection = ADC_EOC_SINGLE_CONV;
  if (HAL_ADC_Init(&hadc1) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure for the selected ADC regular channel its corresponding rank in the sequencer and its sample time.
   */
  sConfig.Channel = ADC_CHANNEL_10;
  sConfig.Rank = 1;
  sConfig.SamplingTime = ADC_SAMPLETIME_15CYCLES;
  if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN ADC1_Init 2 */

  /* USER CODE END ADC1_Init 2 */
}

/**
 * @brief TIM2 Initialization Function
 * @param None
 * @retval None
 */
static void MX_TIM2_Init(void)
{

  /* USER CODE BEGIN TIM2_Init 0 */

  /* USER CODE END TIM2_Init 0 */

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};

  /* USER CODE BEGIN TIM2_Init 1 */

  /* USER CODE END TIM2_Init 1 */
  htim2.Instance = TIM2;
  htim2.Init.Prescaler = 839;
  htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim2.Init.Period = 9999;
  htim2.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim2.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim2) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim2, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim2, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM2_Init 2 */

  /* USER CODE END TIM2_Init 2 */
}

/**
 * @brief USART6 Initialization Function
 * @param None
 * @retval None
 */
static void MX_USART6_UART_Init(void)
{

  /* USER CODE BEGIN USART6_Init 0 */

  /* USER CODE END USART6_Init 0 */

  /* USER CODE BEGIN USART6_Init 1 */

  /* USER CODE END USART6_Init 1 */
  huart6.Instance = USART6;
  huart6.Init.BaudRate = 115200;
  huart6.Init.WordLength = UART_WORDLENGTH_8B;
  huart6.Init.StopBits = UART_STOPBITS_1;
  huart6.Init.Parity = UART_PARITY_NONE;
  huart6.Init.Mode = UART_MODE_TX_RX;
  huart6.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart6.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart6) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART6_Init 2 */

  /* USER CODE END USART6_Init 2 */
}

/**
 * Enable DMA controller clock
 */
static void MX_DMA_Init(void)
{

  /* DMA controller clock enable */
  __HAL_RCC_DMA2_CLK_ENABLE();

  /* DMA interrupt init */
  /* DMA2_Stream0_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA2_Stream0_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(DMA2_Stream0_IRQn);
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
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_1, GPIO_PIN_SET);

  /*Configure GPIO pin : PA0 */
  GPIO_InitStruct.Pin = GPIO_PIN_0;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
  GPIO_InitStruct.Pull = GPIO_PULLDOWN;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pin : PA1 */
  GPIO_InitStruct.Pin = GPIO_PIN_1;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /* EXTI interrupt init*/
  HAL_NVIC_SetPriority(EXTI0_IRQn, 5, 0);
  HAL_NVIC_EnableIRQ(EXTI0_IRQn);

  /* USER CODE BEGIN MX_GPIO_Init_2 */

  /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
  uint32_t now = HAL_GetTick();

  if ((GPIO_Pin == GPIO_PIN_0) && ((now - button_last_tick) > 150U))
  {
    button_last_tick = now;
    pending_presses++;
    press_count++;
  }
}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
  if (huart->Instance == USART6)
  {
    if (rx_index < sizeof(rx_buffer) - 1)
    {
      if (rx_byte != '\r' && rx_byte != '\n')
      {
        rx_buffer[rx_index++] = rx_byte;
        rx_buffer[rx_index] = '\0';
      }
      else
      {
        command_ready = 1;
        rx_buffer[rx_index] = '\0';
      }
    }
    else
    {
      rx_index = 0;
      memset(rx_buffer, 0, sizeof(rx_buffer));
    }
  }
  HAL_UART_Receive_IT(&huart6, &rx_byte, 1);
}

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
  if (htim->Instance == TIM2)
  {
    tim_flag = 1;
  }
}

void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef *hadc)
{
  if (hadc->Instance == ADC1)
  {
    adc_flag = 1;
  }
}
/* USER CODE END 4 */

/**
 * @brief  This function is executed in case of error occurrence.
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
