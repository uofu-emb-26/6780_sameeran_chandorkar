/***************************************** includes */
#include "hal_gpio.h"
#include "main.h"
#include "stm32f0xx_hal.h"
#include <stdint.h>

/***************************************** MACROs */
#define LED_RED_PIN 6U    // PC6
#define LED_BLUE_PIN 7U   // PC7
#define LED_ORANGE_PIN 8U // PC8
#define LED_GREEN_PIN 9U  // PC9

#define USART3_TX_PIN 10U
#define USART3_RX_PIN 11U
#define USART3_PORT GPIOC
#define USART3_AF 1U // Alternate Function 1 for USART3

#define CHECKOFF1 0
#if defined(CHECKOFF1) && CHECKOFF1 == 0
#define CHECKOFF2 1
#endif

/***************************************** function declarations */
void SystemClock_Config(void);
void init_leds(void);
void USART3_Init(void);
void USART3_TransmitChar(char c);
void USART3_TransmitString(const char *str);
char USART3_ReceiveChar(void);
uint32_t GetLedPin(char color);
void PrintMenu(void);
void ProcessCommand(char c);
static void flash_leds(uint32_t led_pin);
void PollUART(void);

void ProcessCommandonIRQ();
static void ActOnIRQCommand(char color_cmd, char action_cmd);

/***************************************** global variables */

#define RX_BUFFER_SIZE 32
volatile uint8_t rx_buffer[RX_BUFFER_SIZE];
volatile uint8_t rx_head = 0;
volatile uint8_t rx_tail = 0;
uint8_t state = 0; // 0 = waiting for color, 1 = waiting for action
char color_cmd = 0;

typedef enum { false, true } bool;

/***************************************** start of file */
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

  flash_leds(LED_RED_PIN);
  PrintMenu();
  flash_leds(LED_GREEN_PIN);

  while (1) {
#if defined(CHECKOFF1) && CHECKOFF1 == 1
    ProcessCommand(USART3_ReceiveChar());
#else
    ProcessCommandonIRQ();
#endif
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

void flash_leds(uint32_t led_pin) {
  GPIO_TogglePin(GPIOC, led_pin);
  HAL_Delay(100);
  GPIO_TogglePin(GPIOC, led_pin);
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

#if defined(CHECKOFF2) && CHECKOFF2 == 1
  // Enable RXNE interrupt in USART3
  USART3->CR1 |= USART_CR1_RXNEIE;

  // Enable USART3_4 interrupt in NVIC
  NVIC_SetPriority(USART3_4_IRQn, 0);
  NVIC_EnableIRQ(USART3_4_IRQn);
#endif
}

void PollUART(void) {
  // Check for Overrun Error and clear it to prevent freezing
  if (USART3->ISR & USART_ISR_ORE) {
    USART3->ICR |= USART_ICR_ORECF;
  }

  // Check if RXNE (Receive Not Empty) is set
  if (USART3->ISR & USART_ISR_RXNE) {
    uint8_t data = (uint8_t)(USART3->RDR);
    uint8_t next_head = (rx_head + 1) % RX_BUFFER_SIZE;
    if (next_head != rx_tail) {
      rx_buffer[rx_head] = data;
      rx_head = next_head;
    }
  }
}

void USART3_TransmitChar(char c) {
  // Wait until TXE (Transmit Data Register Empty) flag is set
  while (!(USART3->ISR & USART_ISR_TXE)) {
#if !defined(CHECKOFF2) || CHECKOFF2 == 0
    PollUART(); // Poll for incoming data while waiting to transmit
#endif
  }

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
  // Wait until data is available in the buffer
  while (rx_head == rx_tail) {
    PollUART();
  }

  char c = (char)rx_buffer[rx_tail];
  rx_tail = (rx_tail + 1) % RX_BUFFER_SIZE;
  return c;
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
#if defined(CHECKOFF1) && CHECKOFF1 == 1
  USART3_TransmitString("=== LED Control ===\r\n");
  USART3_TransmitString("Press r/g/b/o to toggle LEDs\r\n\r\n");
#else
  USART3_TransmitString("\r\n=== LED Control ===\r\n");
  USART3_TransmitString("Usage: [led][action]\r\n\n");
  USART3_TransmitString("led:\r\n"
                        "  r  Red LED\r\n"
                        "  o  Orange LED\r\n"
                        "  b  Blue LED\r\n"
                        "  g  Green LED\r\n"
                        "\r\n");
  USART3_TransmitString("action:\r\n"
                        "  0  OFF\r\n"
                        "  1  ON\r\n"
                        "  2  TOGGLE\r\n"
                        "\r\n");
  USART3_TransmitString("CMD$ ");
#endif
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

void ProcessCommandonIRQ() {
  while (rx_head != rx_tail) {
    char c = (char)rx_buffer[rx_tail];
    rx_tail = (rx_tail + 1) % RX_BUFFER_SIZE;

    bool waitForUserInput = true;
        if (state == 0) {
      // state 0, waiting for color character
      uint32_t led_pin = GetLedPin(c);

      if (led_pin != 0xFF) // valid color
      {
        color_cmd = c;
            state = 1;
        USART3_TransmitChar(c); // Echo the character
        waitForUserInput = false;
          } else {
        // Invalid color
        USART3_TransmitString("\r\nError: Invalid color! Use r/g/b/o\r\n");
          }
        } else {
      // stat1 1, waiting for action character (0, 1, or 2)
      if (c >= '0' && c <= '2') {
        USART3_TransmitChar(c); // Echo the character
        ActOnIRQCommand(color_cmd, c);
            state = 0;
          } else {
        // Invalid action
        USART3_TransmitString("\r\nError: Invalid action! Use 0/1/2\r\n");
            state = 0;
          }
        }
    if (waitForUserInput) {
      USART3_TransmitString("\r\nCMD$ ");
    }
  }
}
void ActOnIRQCommand(char color, char action) {
  uint32_t led_pin = GetLedPin(color);

  const char *color_name;
  switch (color) {
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
  default:
    return;
  }

  USART3_TransmitString("\r\n");
  USART3_TransmitString(color_name);

  switch (action) {
  case '0':
    GPIO_WritePin(GPIOC, led_pin, GPIO_PIN_RESET);
    USART3_TransmitString(" LED Off\r\n");
    break;
  case '1':
    GPIO_WritePin(GPIOC, led_pin, GPIO_PIN_SET);
    USART3_TransmitString(" LED On\r\n");
    break;
  case '2':
    GPIO_TogglePin(GPIOC, led_pin);
    USART3_TransmitString(" LED Toggeled\r\n");
    break;
  }
}
void USART3_4_IRQHandler(void) {
  // Check for Overrun Error and clear it to prevent infinite loop
  if (USART3->ISR & USART_ISR_ORE) {
    USART3->ICR |= USART_ICR_ORECF;
  }

  // Check if RXNE (Receive Not Empty) caused the interrupt
  if (USART3->ISR & USART_ISR_RXNE) {
    uint8_t data =
        (uint8_t)(USART3->RDR); // Reading RDR automatically clears RXNE
    uint8_t next_head = (rx_head + 1) % RX_BUFFER_SIZE;
    if (next_head != rx_tail) {
      rx_buffer[rx_head] = data;
      rx_head = next_head;
    }
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