#include "hal_gpio.h"
#include "main.h"
#include "stm32f0xx_hal.h"

#define LED_RED_PIN 6    // PC6
#define LED_BLUE_PIN 7   // PC7
#define LED_ORANGE_PIN 8 // PC8
#define LED_GREEN_PIN 9  // PC9

#define USART3_TX_PIN 10
#define USART3_RX_PIN 11
#define USART3_PORT GPIOC
#define USART3_AF 1 // Alternate Function 1 for USART3

void SystemClock_Config(void);
void init_leds(void);
void USART3_Init(void);
void USART3_TransmitChar(char c);
void USART3_TransmitString(const char *str);
char USART3_ReceiveChar(void);
uint32_t GetLedPin(char color);
void PrintMenu(void);
void ProcessCommand(char c);

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
  init_leds();
  USART3_Init();

  PrintMenu();

  while (1) {
    char c = USART3_ReceiveChar();
    ProcessCommand(c);
  }
  return 0;
}

void init_leds(void) {
  GPIO_EnableClock(GPIOC);

  GPIO_Init(GPIOC, LED_RED_PIN, GPIO_MODE_OUTPUT_PP, GPIO_SPEED_FREQ_LOW,
            GPIO_NOPULL);
  GPIO_Init(GPIOC, LED_BLUE_PIN, GPIO_MODE_OUTPUT_PP, GPIO_SPEED_FREQ_LOW,
            GPIO_NOPULL);
  GPIO_Init(GPIOC, LED_ORANGE_PIN, GPIO_MODE_OUTPUT_PP, GPIO_SPEED_FREQ_LOW,
            GPIO_NOPULL);
  GPIO_Init(GPIOC, LED_GREEN_PIN, GPIO_MODE_OUTPUT_PP, GPIO_SPEED_FREQ_LOW,
            GPIO_NOPULL);
}

void USART3_Init(void) {
  // Enable GPIO clock for USART pins
  GPIO_EnableClock(USART3_PORT);

  // Enable USART3 clock in RCC
  RCC->APB1ENR |= RCC_APB1ENR_USART3EN;
  //__HAL_RCC_USART3_CLK_ENABLE();

  // Configure GPIO pins for alternate function
  GPIO_Init(USART3_PORT, USART3_TX_PIN, GPIO_MODE_AF_PP, GPIO_SPEED_FREQ_HIGH,
            GPIO_NOPULL);
  GPIO_Init(USART3_PORT, USART3_RX_PIN, GPIO_MODE_AF_PP, GPIO_SPEED_FREQ_HIGH,
            GPIO_NOPULL);

  // Set alternate function number (AF1 for USART3 on PC10/PC11)
  GPIO_SetAlternateFunction(USART3_PORT, USART3_TX_PIN, USART3_AF);
  GPIO_SetAlternateFunction(USART3_PORT, USART3_RX_PIN, USART3_AF);

  // Configure baud rate (115200)
  //  BRR = fCLK / baud_rate
  USART3->BRR = HAL_RCC_GetHCLKFreq() / 115200;

  // Enable transmitter (TE) and receiver (RE) in CR1
  USART3->CR1 |= USART_CR1_TE | USART_CR1_RE;

  // Enable USART peripheral
  USART3->CR1 |= USART_CR1_UE;
}

void USART3_TransmitChar(char c) {
  // Wait until TXE (Transmit Data Register Empty) flag is set
  while (!(USART3->ISR & USART_ISR_TXE))
    ;

  // Write character to transmit data register
  USART3->TDR = c;
}

void USART3_TransmitString(const char *str) {
  while (*str != '\0') {
    USART3_TransmitChar(*str);
    str++;
  }
}

char USART3_ReceiveChar(void) {
  // Wait until RXNE (Receive Data Register Not Empty) flag is set
  while (!(USART3->ISR & USART_ISR_RXNE))
    ;

  // Read and return received character (reading RDR clears RXNE)
  return (char)(USART3->RDR);
}

uint32_t GetLedPin(char color) {
  switch (color) {
  case 'r':
  case 'R':
    return LED_RED_PIN;
  case 'g':
  case 'G':
    return LED_GREEN_PIN;
  case 'b':
  case 'B':
    return LED_BLUE_PIN;
  case 'o':
  case 'O':
    return LED_ORANGE_PIN;
  default:
    return 0xFF; // Invalid
  }
}

void PrintMenu(void) {
  USART3_TransmitString("\r\n=== LED Control ===\r\n");
  USART3_TransmitString("Press r/g/b/o to toggle LEDs\r\n\r\n");
}

void ProcessCommand(char c) {
  // Ignore whitespace characters (CR, LF, Space, Tab)
  if (c == '\r' || c == '\n' || c == ' ' || c == '\t') {
    return;
  }

  uint32_t led_pin = GetLedPin(c);

  if (led_pin != 0xFF) {
    // Valid LED color, toggle it
    GPIO_TogglePin(GPIOC, led_pin);

    const char *color_name = "";
    switch (c) {
    case 'r':
    case 'R':
      color_name = "Red";
      break;
    case 'g':
    case 'G':
      color_name = "Green";
      break;
    case 'b':
    case 'B':
      color_name = "Blue";
      break;
    case 'o':
    case 'O':
      color_name = "Orange";
      break;
    }

    USART3_TransmitString(color_name);
    USART3_TransmitString(" LED toggled. State: ");

    // Read the pin state to confirm (GPIO_ReadPin reads IDR)
    if (GPIO_ReadPin(GPIOC, led_pin) == GPIO_PIN_SET) {
      USART3_TransmitString("ON\r\n");
    } else {
      USART3_TransmitString("OFF\r\n");
    }
  } else {
    // Invalid character - print error
    USART3_TransmitString("Error: Unknown command '");
    USART3_TransmitChar(c);
    USART3_TransmitString("'\r\n");
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