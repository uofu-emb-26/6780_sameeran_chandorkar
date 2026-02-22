#include "hal_gpio.h"
#include "main.h"
#include "stm32f0xx_hal.h"

// PA0 is user input/button,
// PC6 - RED LED
// PC7 - BLUE LED
// PC8 - ORANGE LED
// PC9 - GREEN LED

static void SystemClock_Config(void);
static void LED_Init(void);
static void Timer2_Init(void);
static void PWM_Init(void);
static void PWM_GPIO_Init(void);
static void PWM_SetDutyCycle(uint32_t ch1_percent, uint32_t ch2_percent);

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

  /* Initialize Peripherals */
  LED_Init();
  Timer2_Init();
  PWM_GPIO_Init();
  PWM_Init();

  while (1) {

    for (int i = 0; i < 100; i++) {
      // ch1 --> 0 = bright
      // ch2 --> 0 = dim
      PWM_SetDutyCycle(i, i);
      HAL_Delay(10);
    }
    for (int i = 100; i > 0; i--) {
      // ch1 --> 0 = bright
      // ch2 --> 0 = dim
      PWM_SetDutyCycle(i, i);
      HAL_Delay(10);
    }
  }
  return 0;
}

static void LED_Init(void) {
  // Enable GPIOC clock
  GPIO_EnableClock(GPIOC);

  // Configure PC8 (green LED) as output
  GPIO_Init(GPIOC, 8, GPIO_MODE_OUTPUT_PP, GPIO_SPEED_FREQ_LOW, GPIO_NOPULL);

  // Configure PC9 (orange LED) as output
  GPIO_Init(GPIOC, 9, GPIO_MODE_OUTPUT_PP, GPIO_SPEED_FREQ_LOW, GPIO_NOPULL);

  // Set initial state, green ON, orange OFF
  GPIO_WritePin(GPIOC, 8, GPIO_PIN_SET);
  GPIO_WritePin(GPIOC, 9, GPIO_PIN_RESET);
}

static void Timer2_Init(void) {
  // Enable TIM2 clock in RCC
  RCC->APB1ENR |= RCC_APB1ENR_TIM2EN;

  // Set prescaler: 8 MHz / 8000 = 1 kHz timer frequency
  TIM2->PSC = 7999;

  // Set auto-reload: count to 250 for 4 Hz (250 ms period)
  TIM2->ARR = 250;

  // Enable Update Interrupt in DIER
  TIM2->DIER |= TIM_DIER_UIE;

  // Enable TIM2 interrupt in NVIC
  NVIC_SetPriority(TIM2_IRQn, 2);
  NVIC_EnableIRQ(TIM2_IRQn);

  // Enable the timer (start counting)
  TIM2->CR1 |= TIM_CR1_CEN;
}

void TIM2_IRQHandler(void) {
  // Check if update interrupt flag is set
  if (TIM2->SR & TIM_SR_UIF) {
    // Clear the update interrupt flag (write 0 to clear)
    TIM2->SR &= ~TIM_SR_UIF;

    // Toggle green LED (PC8)
    GPIO_TogglePin(GPIOC, 8);

    // Toggle orange LED (PC9)
    GPIO_TogglePin(GPIOC, 9);
  }
}

static void PWM_GPIO_Init(void) {
  RCC->AHBENR |= RCC_AHBENR_GPIOCEN;

  // Set PC6 and PC7 to Alternate Function mode
  GPIOC->MODER &= ~(GPIO_MODER_MODER6 | GPIO_MODER_MODER7);
  GPIOC->MODER |= (GPIO_MODER_MODER6_1 | GPIO_MODER_MODER7_1);

  GPIOC->OTYPER &= ~(GPIO_OTYPER_OT_6 | GPIO_OTYPER_OT_7);

  // AF0 for TIM3_CH1 (PC6) and TIM3_CH2 (PC7)
  GPIOC->AFR[0] &= ~((0xFUL << 24) | (0xFUL << 28));
  /* AF0 = 0, so no need to set any bits */
}

static void PWM_SetDutyCycle(uint32_t ch1_percent, uint32_t ch2_percent) {
  /* Convert percentage to CCR value (ARR = 10000) */
  TIM3->CCR1 = (ch1_percent * 10000) / 100;
  TIM3->CCR2 = (ch2_percent * 10000) / 100;
}

static void PWM_Init(void) {
  RCC->APB1ENR |= RCC_APB1ENR_TIM3EN;

  // 800 Hz PWM
  TIM3->PSC = 0;
  TIM3->ARR = 10000;

  TIM3->CCMR1 = 0;

  // Channel 1 (PC6 red): PWM Mode 2
  TIM3->CCMR1 |= (TIM_CCMR1_OC1M_2 | TIM_CCMR1_OC1M_1 | TIM_CCMR1_OC1M_0);
  TIM3->CCMR1 |= TIM_CCMR1_OC1PE;

  // Channel 2 (PC7 blue): PWM Mode 1
  TIM3->CCMR1 |= (TIM_CCMR1_OC2M_2 | TIM_CCMR1_OC2M_1);
  TIM3->CCMR1 |= TIM_CCMR1_OC2PE;

  TIM3->CCER |= TIM_CCER_CC1E;
  TIM3->CCER |= TIM_CCER_CC2E;

  // 20% duty cycle
  // TIM3->CCR1 = 2000;
  // TIM3->CCR2 = 2000;
  TIM3->CR1 |= TIM_CR1_CEN;
}

/**
 * @brief System Clock Configuration
 * @retval None
 */
static void SystemClock_Config(void) {
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