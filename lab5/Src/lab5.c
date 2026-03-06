#include "hal_gpio.h"
#include "main.h"
#include "stm32f0xx_hal.h"

void SystemClock_Config(void);
#ifndef CHECKOFF_1
#define CHECKOFF_1 0
#if defined(CHECKOFF_1) && CHECKOFF_1 == 0
#define CHECKOFF_2 1
#endif
#endif

// L3GD20 defines
#define GYRO_ADDR 0x69 // SDO pin is high (PB14 set high)
#define WHO_AM_I_REG 0x0F
#define WHO_AM_I_VAL 0xD3
#define CTRL_REG1 0x20
#define OUT_X_L 0xA8 // MSB set for auto-increment multi-byte read
#define OUT_Y_L 0xAA

#define LED_RED_PIN 6
#define LED_BLUE_PIN 7
#define LED_ORANGE_PIN 8
#define LED_GREEN_PIN 9

// Threshold to ignore noise
#define GYRO_THRESHOLD 1000

static void App_GPIO_Init(void);
static void App_I2C_Init(void);
static void LED_SetAll(uint8_t r, uint8_t b, uint8_t o, uint8_t g);
#if defined(CHECKOFF_2) && CHECKOFF_2 == 1
static void Gyro_Init(void);
#endif

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

  App_GPIO_Init();
  App_I2C_Init();

  uint8_t whoami;
  int8_t all_leds_on = 0;

  while (1) {
    whoami = 0;
    I2C_ReadReg(GYRO_ADDR, WHO_AM_I_REG, &whoami, 1);

    if (whoami == WHO_AM_I_VAL) {
      // Blink all LEDs briefly to signal success
      if (!all_leds_on) {
        LED_SetAll(1, 1, 1, 1);
        HAL_Delay(100);
        LED_SetAll(0, 0, 0, 0);
        HAL_Delay(100);
        all_leds_on = 1;
      }

#if defined(CHECKOFF_2) && CHECKOFF_2 == 1
      static uint8_t raw[2];
      static int16_t x_val, y_val;
      Gyro_Init();

      // Read X axis (2 bytes, auto-increment)
      I2C_ReadReg(GYRO_ADDR, OUT_X_L, raw, 2);
      x_val = (int16_t)((raw[1] << 8) | raw[0]);

      // Read Y axis (2 bytes, auto-increment)
      I2C_ReadReg(GYRO_ADDR, OUT_Y_L, raw, 2);
      y_val = (int16_t)((raw[1] << 8) | raw[0]);

      // Clear all LEDs first
      LED_SetAll(0, 0, 0, 0);

      if (x_val > GYRO_THRESHOLD)
        GPIO_WritePin(GPIOC, LED_GREEN_PIN, GPIO_PIN_SET);
      else if (x_val < -GYRO_THRESHOLD)
        GPIO_WritePin(GPIOC, LED_ORANGE_PIN, GPIO_PIN_SET);

      if (y_val > GYRO_THRESHOLD)
        GPIO_WritePin(GPIOC, LED_RED_PIN, GPIO_PIN_SET);
      else if (y_val < -GYRO_THRESHOLD)
        GPIO_WritePin(GPIOC, LED_BLUE_PIN, GPIO_PIN_SET);

      HAL_Delay(100); // Read at ~10 Hz, sensor outputs at 95 Hz default
#endif
    } else {
      // if WHO_AM_I mismatch, flash red repeatedly
      if (all_leds_on) {
        LED_SetAll(0, 0, 0, 0);
        all_leds_on = 0;
      }
      GPIO_TogglePin(GPIOC, LED_RED_PIN);
      HAL_Delay(100);
    }
  }
  return -1;
}

static void App_GPIO_Init(void) {
  GPIO_EnableClock(GPIOB);
  GPIO_EnableClock(GPIOC);

  // PB11
  GPIO_Init(GPIOB, 11, GPIO_MODE_AF_OD, GPIO_SPEED_FREQ_HIGH, GPIO_PULLUP);
  GPIO_SetAlternateFunction(GPIOB, 11, 1); // AF1 = I2C2_SDA

  // PB13 = I2C2_SCL
  GPIO_Init(GPIOB, 13, GPIO_MODE_AF_OD, GPIO_SPEED_FREQ_HIGH, GPIO_PULLUP);
  GPIO_SetAlternateFunction(GPIOB, 13, 5); // AF5 = I2C2_SCL

  // PB14 = SA0 (slave address bit)
  GPIO_Init(GPIOB, 14, GPIO_MODE_OUTPUT_PP, GPIO_SPEED_FREQ_LOW, GPIO_NOPULL);
  GPIO_WritePin(GPIOB, 14, GPIO_PIN_SET);

  // PC0 = CS/mode select
  GPIO_Init(GPIOC, 0, GPIO_MODE_OUTPUT_PP, GPIO_SPEED_FREQ_LOW, GPIO_NOPULL);
  GPIO_WritePin(GPIOC, 0, GPIO_PIN_SET);

  // LEDs: PC6, PC7, PC8, PC9
  GPIO_Init(GPIOC, LED_RED_PIN, GPIO_MODE_OUTPUT_PP, GPIO_SPEED_FREQ_LOW,
            GPIO_NOPULL);
  GPIO_Init(GPIOC, LED_BLUE_PIN, GPIO_MODE_OUTPUT_PP, GPIO_SPEED_FREQ_LOW,
            GPIO_NOPULL);
  GPIO_Init(GPIOC, LED_ORANGE_PIN, GPIO_MODE_OUTPUT_PP, GPIO_SPEED_FREQ_LOW,
            GPIO_NOPULL);
  GPIO_Init(GPIOC, LED_GREEN_PIN, GPIO_MODE_OUTPUT_PP, GPIO_SPEED_FREQ_LOW,
            GPIO_NOPULL);
}

static void App_I2C_Init(void) {
  I2C2_RCC_CLK_Enable();
  I2C2_Init();
}
#if defined(CHECKOFF_2) && CHECKOFF_2 == 1
static void Gyro_Init(void) {
  // CTRL_REG1: PD=1 (normal mode), Yen=1, Xen=1, all other bits 0
  // Bit pattern: 0b00001011 = 0x0B
  uint8_t ctrl = 0x0B;
  I2C_WriteReg(GYRO_ADDR, CTRL_REG1, &ctrl, 1);
}
#endif

static void LED_SetAll(uint8_t r, uint8_t b, uint8_t o, uint8_t g) {
  GPIO_WritePin(GPIOC, LED_RED_PIN, r ? GPIO_PIN_SET : GPIO_PIN_RESET);
  GPIO_WritePin(GPIOC, LED_BLUE_PIN, b ? GPIO_PIN_SET : GPIO_PIN_RESET);
  GPIO_WritePin(GPIOC, LED_ORANGE_PIN, o ? GPIO_PIN_SET : GPIO_PIN_RESET);
  GPIO_WritePin(GPIOC, LED_GREEN_PIN, g ? GPIO_PIN_SET : GPIO_PIN_RESET);
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
