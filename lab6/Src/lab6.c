#include "main.h"
#include "stm32f0xx_hal.h"
#include "hal_gpio.h"

#define LED_RED_PIN 6
#define LED_BLUE_PIN 7
#define LED_ORANGE_PIN 8
#define LED_GREEN_PIN 9

#define ANALOG_IN_0 0
#define ANALOG_OUT_4 4

#define THRESH_LED0 64u
#define THRESH_LED1 128u
#define THRESH_LED2 192u
#define THRESH_LED3 224u

void SystemClock_Config(void);
static void LED_Init(void);
static void ADC_Init(void);
static void DAC_Init(void);

const uint8_t SINE_TABLE[32] = {
    127,151,175,197,216,232,244,251,254,251,244,
    232,216,197,175,151,127,102,78,56,37,21,9,2,0,2,9,21,37,56,78,102
};
   

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

  LED_Init();
  ADC_Init();
  DAC_Init();
 
  uint32_t sine_index = 0;
 
  while (1)
  {
    // 6.1 checkoff
    uint32_t adc_value = ADC1->DR;   /* 8-bit result: 0–255 */

    GPIO_WritePin(GPIOC, LED_RED_PIN,
                  (adc_value > THRESH_LED0) ? GPIO_PIN_SET : GPIO_PIN_RESET);
    GPIO_WritePin(GPIOC, LED_BLUE_PIN,
            (adc_value > THRESH_LED2) ? GPIO_PIN_SET : GPIO_PIN_RESET);
    GPIO_WritePin(GPIOC, LED_ORANGE_PIN,
            (adc_value > THRESH_LED1) ? GPIO_PIN_SET : GPIO_PIN_RESET);
    GPIO_WritePin(GPIOC, LED_GREEN_PIN,
            (adc_value > THRESH_LED3) ? GPIO_PIN_SET : GPIO_PIN_RESET);
 
    // 6.2 checkoff
    DAC->DHR8R1 = SINE_TABLE[sine_index];
    sine_index = (sine_index + 1) & 0x1F; /* Optimized modulo 32 using bitwise AND */
 
    HAL_Delay(1);   /* 1 ms between samples */
  }
  return -1;
}

static void LED_Init(void)
{
    GPIO_EnableClock(GPIOC);
    GPIO_Init(GPIOC, LED_RED_PIN, GPIO_MODE_OUTPUT_PP, GPIO_SPEED_FREQ_LOW, GPIO_NOPULL);
    GPIO_Init(GPIOC, LED_BLUE_PIN, GPIO_MODE_OUTPUT_PP, GPIO_SPEED_FREQ_LOW, GPIO_NOPULL);
    GPIO_Init(GPIOC, LED_ORANGE_PIN, GPIO_MODE_OUTPUT_PP, GPIO_SPEED_FREQ_LOW, GPIO_NOPULL);
    GPIO_Init(GPIOC, LED_GREEN_PIN, GPIO_MODE_OUTPUT_PP, GPIO_SPEED_FREQ_LOW, GPIO_NOPULL);
}

static void ADC_Init(void)
{
    // Configure PC0 as analog input
    GPIO_EnableClock(GPIOC);
    GPIO_Init(GPIOC, ANALOG_IN_0, GPIO_MODE_ANALOG, GPIO_SPEED_FREQ_LOW, GPIO_NOPULL);
 
    // Enable ADC1 peripheral clock
    RCC->APB2ENR |=  RCC_APB2ENR_ADC1EN;
 
    // Configure ADC:
    ADC1->CFGR1 = ADC_CFGR1_RES_1   /* bits [4:3] = 10 = 8 bit */
                | ADC_CFGR1_CONT;    /* bit 13 = continuous mode  */
 
    // Select channel 10 (PC0 = ADC_IN10)
    ADC1->CHSELR = ADC_CHSELR_CHSEL10;
 
    // Calibration 
    if (ADC1->CR & ADC_CR_ADEN)
    {
        ADC1->CR |= ADC_CR_ADDIS;
        while (ADC1->CR & ADC_CR_ADEN);   /* wait until disabled */
    }
 
    ADC1->CR |= ADC_CR_ADCAL;             /* trigger calibration */
    while (ADC1->CR & ADC_CR_ADCAL); 
 
    // Enable ADC
    ADC1->CR |= ADC_CR_ADEN;
    while (!(ADC1->ISR & ADC_ISR_ADRDY)); /* wait for ready flag */
 
    //Start continuous conversions
    ADC1->CR |= ADC_CR_ADSTART;
}

static void DAC_Init(void)
{
    // Configure PA4 as analog output
    GPIO_EnableClock(GPIOA);
    GPIO_Init(GPIOA, ANALOG_OUT_4, GPIO_MODE_ANALOG, GPIO_SPEED_FREQ_LOW, GPIO_NOPULL);
 
    // Enable DAC peripheral clock
    RCC->APB1ENR |= RCC_APB1ENR_DACEN;
 
    // Enable DAC channel 1 with no hardware trigger
    DAC->CR = DAC_CR_EN1;
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
