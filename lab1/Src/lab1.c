#include "main.h"
#include "stm32f0xx_hal.h"
#include "stm32f072xb.h"
#include "assert.h"
#include "hal_gpio.h"
//PA0 is user input/button,
//PC6 - RED LED
//PC7 - BLUE LED
//PC8 - ORANGE LED
//PC9 - GREEN LED
void SystemClock_Config(void);
void HAL_RCC_GPIOC_CLK_Enable(void);

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{
  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* Configure the system clock */
  SystemClock_Config();

  
  HAL_RCC_GPIOC_CLK_Enable();

  GPIO_InitTypeDef initStr = {GPIO_PIN_6 | GPIO_PIN_7 | GPIO_PIN_8 | GPIO_PIN_9, GPIO_MODE_OUTPUT_PP, GPIO_SPEED_FREQ_LOW,GPIO_NOPULL};
  GPIO_InitTypeDef initBtn = {GPIO_PIN_0, GPIO_MODE_INPUT, GPIO_SPEED_FREQ_LOW, GPIO_PULLDOWN};

  My_HAL_GPIO_Init(GPIOC, &initStr);
  My_HAL_GPIO_Init(GPIOA, &initBtn);
  
  My_HAL_GPIO_WritePin(GPIOC,GPIO_PIN_6,GPIO_PIN_SET);
  My_HAL_GPIO_WritePin(GPIOC,GPIO_PIN_7,GPIO_PIN_RESET);
  My_HAL_GPIO_WritePin(GPIOC,GPIO_PIN_8,GPIO_PIN_RESET);
  GPIO_PinState buttonCurrentState = GPIO_PIN_RESET;
  GPIO_PinState buttonLastState = GPIO_PIN_RESET;

  while (1)
  {
    buttonCurrentState = My_HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_0);
    if ((buttonCurrentState == GPIO_PIN_SET && buttonLastState == GPIO_PIN_RESET)/* || (buttonCurrentState == GPIO_PIN_RESET && buttonLastState == GPIO_PIN_SET)*/)
    {
      My_HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_6 | GPIO_PIN_7);
      buttonLastState = buttonCurrentState;
    }
    else
    {
      buttonLastState = buttonCurrentState;
    }
  }
  return -1;
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_HSI;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* User can add their own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
}

void HAL_RCC_GPIOC_CLK_Enable(void)
{
    // Enable CLK for GPIOC
    RCC->AHBENR |= RCC_AHBENR_GPIOCEN;
    // Enable CLK for GPIOA
    RCC->AHBENR |= RCC_AHBENR_GPIOAEN;
    RCC->APB2ENR |=RCC_APB2ENR_SYSCFGCOMPEN;
};

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
  /* User can add their own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
}
#endif /* USE_FULL_ASSERT */
