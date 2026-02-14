#include "hal_gpio.h"
#include "main.h"
#include "stm32f0xx_hal.h"

#define LED_RED 6    // PC6
#define LED_BLUE 7   // PC7
#define LED_ORANGE 8 // PC8
#define LED_GREEN 9  // PC9
#define BUTTON 0     // PA0

void SystemClock_Config(void);
void init_LEDs(void);
void init_button_interrupt(void);

/**
 * @brief  The application entry point.
 * @retval int
 */
int main(void) {
  /* Reset of all peripherals, Initializes the Flash interface and the Systick.
   */
  HAL_Init();
  /* Configure the system clock */
  SystemClock_Config();

  init_LEDs();
  init_button_interrupt();

  while (1) {
    BSP_GPIO_TogglePin(GPIOC, LED_RED);
    HAL_Delay(500);
  }
  return -1;
}

void init_LEDs(void) {
  BSP_GPIO_EnableClock(GPIOC);

  // Configure each LED: output, push-pull, low speed, no pull
  BSP_GPIO_Init(GPIOC, LED_RED, GPIO_MODE_OUTPUT_PP, GPIO_SPEED_LOW,
                GPIO_NOPULL);
  BSP_GPIO_Init(GPIOC, LED_BLUE, GPIO_MODE_OUTPUT_PP, GPIO_SPEED_LOW,
                GPIO_NOPULL);
  BSP_GPIO_Init(GPIOC, LED_ORANGE, GPIO_MODE_OUTPUT_PP, GPIO_SPEED_LOW,
                GPIO_NOPULL);
  BSP_GPIO_Init(GPIOC, LED_GREEN, GPIO_MODE_OUTPUT_PP, GPIO_SPEED_LOW,
                GPIO_NOPULL);
}

void init_button_interrupt(void) {
  BSP_GPIO_EnableClock(GPIOA);

  // Configure PA0: input, low speed, pull-down
  BSP_GPIO_Init(GPIOA, BUTTON, GPIO_MODE_INPUT, GPIO_SPEED_LOW, GPIO_PULLDOWN);

  // Configure EXTI: rising edge trigger on PA0
  BSP_EXTI_Init(GPIOA, BUTTON, 1, 0); // rising=1, falling=0

  // Enable interrupt with priority 1
  BSP_EXTI_EnableIRQ(BUTTON, 1);
}

void EXTI0_1_IRQHandler(void) {
  // Check if line 0 triggered
  if (EXTI->PR & (1 << BUTTON)) {
    // Toggle green and orange LEDs
    BSP_GPIO_TogglePin(GPIOC, LED_GREEN);
    BSP_GPIO_TogglePin(GPIOC, LED_ORANGE);

    // Clear pending flag - REQUIRED!
    BSP_EXTI_ClearFlag(BUTTON);
  }
}

/**
 * @brief System Clock Configuration
 * @retval None
 */
void SystemClock_Config(void) {
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Initializes the RCC Oscillators according to the specified parameters
   * in the RCC_OscInitTypeDef structure.
   */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
   */
  RCC_ClkInitStruct.ClockType =
      RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_PCLK1;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_HSI;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK) {
    Error_Handler();
  }
}

/**
 * @brief  This function is executed in case of error occurrence.
 * @retval None
 */
void Error_Handler(void) {
  /* User can add their own implementation to report the HAL error return state
   */
  __disable_irq();
  while (1) {
  }
}

#ifdef USE_FULL_ASSERT
/**
 * @brief  Reports the name of the source file and the source line number
 *         where the assert_param error has occurred.
 * @param  file: pointer to the source file name
 * @param  line: assert_param error line source number
 * @retval None
 */
void assert_failed(uint8_t *file, uint32_t line) {
  /* User can add their own implementation to report the file name and line
     number, ex: printf("Wrong parameters value: file %s on line %d\r\n", file,
     line) */
}
#endif /* USE_FULL_ASSERT */