#include "hal_gpio.h"

void GPIO_Init(GPIO_TypeDef *GPIOx, uint32_t pin, uint32_t mode, uint32_t speed,
               uint32_t pull) {
  uint32_t position = pin * 2; // Position for 2-bit fields

  // Extract the actual mode and output type from the combined mode parameter
  uint32_t gpio_mode = (mode & GPIO_MODE) >> GPIO_MODE_Pos;       // Bits 0-1
  uint32_t output_type = (mode & OUTPUT_TYPE) >> OUTPUT_TYPE_Pos; // Bit 4

  // Configure Mode, 2 bits per pin
  GPIOx->MODER &= ~(0x3UL << position);
  GPIOx->MODER |= (gpio_mode << position);

  // Configure Output Type (only relevant for output modes)
  if (gpio_mode == GPIO_MODE_OUTPUT_PP || gpio_mode == GPIO_MODE_AF_PP ||
      gpio_mode == GPIO_MODE_OUTPUT_OD || gpio_mode == GPIO_MODE_AF_OD) {
    GPIOx->OTYPER &= ~(0x1UL << pin);
    GPIOx->OTYPER |= (output_type << pin);
  }

  // Configure Speed, 2 bits per pin
  GPIOx->OSPEEDR &= ~(0x3UL << position);
  GPIOx->OSPEEDR |= (speed << position);

  // Configure pull-up/pull-down, 2 bits per pin
  GPIOx->PUPDR &= ~(0x3UL << position);
  GPIOx->PUPDR |= (pull << position);
}

void GPIO_DeInit(GPIO_TypeDef *GPIOx, uint32_t pin) {
  // Reset logic can be implemented here if needed
  // Typically involves resetting registers to default values
}

uint32_t GPIO_ReadPin(GPIO_TypeDef *GPIOx, uint32_t pin) {
  if (GPIOx->IDR & (0x1UL << pin)) {
    return GPIO_PIN_SET;
  }
  return GPIO_PIN_RESET;
}

void GPIO_WritePin(GPIO_TypeDef *GPIOx, uint32_t pin, uint32_t state) {
  if (state == GPIO_PIN_SET) {
    GPIOx->BSRR = (0x1UL << pin); // Lower 16 bits to SET
  } else {
    GPIOx->BSRR = (0x1UL << (pin + 16)); // Upper 16 bits to RESET
  }
}

/**
 * @brief
 */
void GPIO_TogglePin(GPIO_TypeDef *GPIOx, uint32_t pin) {
  GPIOx->ODR ^= (0x1UL << pin);
}

/**
 * @brief Helper to calculate port index (A=0, B=1, etc.) based on memory
 * address. Assumes ports are spaced by 0x400 bytes, which is standard for
 * STM32F0
 */
static inline uint32_t GPIO_GetPortIndex(GPIO_TypeDef *port) {
  return ((uint32_t)port - (uint32_t)GPIOA) / 0x400;
}

/**
 * @brief Enable GPIO clock in RCC
 */
void GPIO_EnableClock(GPIO_TypeDef *GPIOx) {
  // On STM32F0, GPIO clock enable bits in AHBENR start at bit 17 (GPIOA)
  // and are contiguous for A, B, C, D, E, F.
  // GPIOA_EN = Bit 17
  // GPIOB_EN = Bit 18
  // ...
  uint32_t port_idx = GPIO_GetPortIndex(GPIOx);

  // 17 is the bit position for GPIOAEN in RCC->AHBENR
  RCC->AHBENR |= (1UL << (17 + port_idx));
}

/**
 * @brief Configure EXTI for external interrupt on a pin
 */
void EXTI_Config(GPIO_TypeDef *GPIOx, uint32_t pin, uint32_t rising_edge,
                 uint32_t falling_edge) {
  // Determine port number
  uint32_t port_index = GPIO_GetPortIndex(GPIOx);

  // Enable SYSCFG clock
  __HAL_RCC_SYSCFG_CLK_ENABLE();

  // Configure SYSCFG EXTICR to route pin to EXTI
  // EXTICR[0] = pins 0-3, EXTICR[1] = pins 4-7, etc.
  uint32_t register_index = pin / 4;
  uint32_t bit_position = (pin % 4) * 4;

  SYSCFG->EXTICR[register_index] &= ~(0xFUL << bit_position);     // Clear
  SYSCFG->EXTICR[register_index] |= (port_index << bit_position); // Set port

  // Configure trigger edges
  if (rising_edge)
    EXTI->RTSR |= (1UL << pin);
  else
    EXTI->RTSR &= ~(1UL << pin);

  if (falling_edge)
    EXTI->FTSR |= (1UL << pin);
  else
    EXTI->FTSR &= ~(1UL << pin);

  // Unmask/enable interrupt on this line
  EXTI->IMR |= (1UL << pin);
}

/**
 * @brief Enable EXTI interrupt in NVIC
 */
void EXTI_EnableInterrupt(uint32_t pin, uint32_t priority) {
  IRQn_Type irq_number;

  // Map EXTI line to NVIC IRQ
  if (pin <= 1)
    irq_number = EXTI0_1_IRQn;
  else if (pin <= 3)
    irq_number = EXTI2_3_IRQn;
  else
    irq_number = EXTI4_15_IRQn;

  NVIC_SetPriority(irq_number, priority);
  NVIC_EnableIRQ(irq_number);
}

/**
 * @brief Clear EXTI pending flag (call this in your ISR!)
 */
void EXTI_ClearPending(uint32_t pin) {
  EXTI->PR = (1UL << pin); // Write 1 to clear
}