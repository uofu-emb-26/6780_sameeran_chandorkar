#include "hal_gpio.h"
#include <stdint.h>

// Helper to calculate port index (A=0, B=1, etc.) based on memory address
// Assumes ports are spaced by 0x400 bytes, which is standard for STM32F0
static inline uint32_t GPIO_GetPortIndex(GPIO_TypeDef *port) {
  return ((uint32_t)port - (uint32_t)GPIOA) / 0x400;
}

void BSP_GPIO_Init(GPIO_TypeDef *port, uint32_t pin, uint32_t mode,
                   uint32_t speed, uint32_t pull) {
  uint32_t pin_2bit = pin * 2; // Position for 2-bit fields

  // Extract the actual mode and output type from the combined mode parameter
  // GPIO_MODE mask is usually 0x3, Output Type is bit 4
  uint32_t gpio_mode = (mode & GPIO_MODE) >> GPIO_MODE_Pos;       // Bits 0-1
  uint32_t output_type = (mode & OUTPUT_TYPE) >> OUTPUT_TYPE_Pos; // Bit 4

  // Configure Mode, 2 bits per pin
  port->MODER &= ~(0x3UL << pin_2bit);
  port->MODER |= (gpio_mode << pin_2bit);

  // Configure Output Type (only relevant for output modes)
  if (gpio_mode == 0x1 || gpio_mode == 0x2) { // OUTPUT or AF mode
    port->OTYPER &= ~(0x1UL << pin);
    port->OTYPER |= (output_type << pin);
  }

  // Configure Speed, 2 bits per pin
  port->OSPEEDR &= ~(0x3UL << pin_2bit);
  port->OSPEEDR |= (speed << pin_2bit);

  // Configure pull-up/pull-down, 2 bits per pin
  port->PUPDR &= ~(0x3UL << pin_2bit);
  port->PUPDR |= (pull << pin_2bit);
}

uint32_t BSP_GPIO_ReadPin(GPIO_TypeDef *port, uint32_t pin) {
  if (port->IDR & (0x1UL << pin)) {
    return GPIO_PIN_SET;
  }
  return GPIO_PIN_RESET;
}

void BSP_GPIO_WritePin(GPIO_TypeDef *port, uint32_t pin, uint32_t state) {
  if (state == GPIO_PIN_SET) {
    port->BSRR = (0x1UL << pin); // Lower 16 bits to SET
  } else {
    port->BSRR = (0x1UL << (pin + 16)); // Upper 16 bits to RESET
  }
}

void BSP_GPIO_TogglePin(GPIO_TypeDef *port, uint32_t pin) {
  port->ODR ^= (0x1UL << pin);
}

void BSP_GPIO_EnableClock(GPIO_TypeDef *port) {
  // On STM32F0, GPIO clock enable bits in AHBENR start at bit 17 (GPIOA)
  // and are contiguous for A, B, C, D, E, F.
  // GPIOA_EN = Bit 17
  // GPIOB_EN = Bit 18
  // ...
  uint32_t port_idx = GPIO_GetPortIndex(port);

  // 17 is the bit position for GPIOAEN in RCC->AHBENR
  RCC->AHBENR |= (1UL << (17 + port_idx));
}

// EXTI Configuration Functions (Lab 2)

/**
 * @brief Configure EXTI for external interrupt on a pin
 */
void BSP_EXTI_Init(GPIO_TypeDef *port, uint32_t pin, uint32_t rising_edge,
                   uint32_t falling_edge) {
  // Determine port index (0=A, 1=B, etc.)
  uint32_t port_idx = GPIO_GetPortIndex(port);

  // Enable SYSCFG clock
  RCC->APB2ENR |= RCC_APB2ENR_SYSCFGEN;

  // Configure SYSCFG EXTICR to route pin to EXTI
  // EXTICR[0] = pins 0-3, EXTICR[1] = pins 4-7, etc.
  uint32_t reg_idx = pin / 4;
  uint32_t bit_pos = (pin % 4) * 4;

  SYSCFG->EXTICR[reg_idx] &= ~(0xFUL << bit_pos);   // Clear
  SYSCFG->EXTICR[reg_idx] |= (port_idx << bit_pos); // Set port

  // Configure trigger edges
  if (rising_edge)
    EXTI->RTSR |= (1U << pin);
  else
    EXTI->RTSR &= ~(1U << pin);

  if (falling_edge)
    EXTI->FTSR |= (1U << pin);
  else
    EXTI->FTSR &= ~(1U << pin);

  // Unmask/enable interrupt on this line
  EXTI->IMR |= (1U << pin);
}

/**
 * @brief Enable EXTI interrupt in NVIC
 */
void BSP_EXTI_EnableIRQ(uint32_t pin, uint32_t priority) {
  IRQn_Type irqn;

  // Map EXTI line to NVIC IRQ
  if (pin <= 1)
    irqn = EXTI0_1_IRQn;
  else if (pin <= 3)
    irqn = EXTI2_3_IRQn;
  else
    irqn = EXTI4_15_IRQn;

  NVIC_SetPriority(irqn, priority);
  NVIC_EnableIRQ(irqn);
}

/**
 * @brief Clear EXTI pending flag (call this in your ISR!)
 */
void BSP_EXTI_ClearFlag(uint32_t pin) {
  EXTI->PR = (1U << pin); // Write 1 to clear
}