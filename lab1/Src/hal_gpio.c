#include <stdint.h>
#include <stm32f0xx_hal.h>
#include <stm32f0xx_hal_gpio.h>

void My_HAL_GPIO_Init(GPIO_TypeDef *GPIOx, GPIO_InitTypeDef *GPIO_Init) {
  uint32_t lu32_position;
  uint32_t temp;

  for (lu32_position = 0; lu32_position < 16; lu32_position++) {
    if ((GPIO_Init->Pin) & (1u << lu32_position)) {
      temp = GPIOx->MODER;
      // Clear the desired bits
      temp &= ~(3u << (lu32_position * 2u));
      // Set desired bits
      temp |= ((GPIO_Init->Mode & 0x3u) << (lu32_position * 2u));
      GPIOx->MODER = temp;

      // Check bit 4 of the Mode parameter to see if it's Open-Drain
      temp = GPIOx->OTYPER;
      // Clear the 4th bit
      temp &= ~(1u << lu32_position);
      if ((GPIO_Init->Mode & 0x10u) >> 4u) { // If bit 4 is set
        temp |= (1u << lu32_position);
      }
      GPIOx->OTYPER = temp;

      temp = GPIOx->PUPDR;
      // Clear the desired bits
      temp &= ~(3u << (lu32_position * 2u));
      // Set the desired bits
      temp |= (GPIO_Init->Pull << (lu32_position * 2u));
      GPIOx->PUPDR = temp;

      temp = GPIOx->OSPEEDR;
      // Clear the desired bits
      temp &= ~(3u << (lu32_position * 2u));
      // Set the desired bits
      temp |= (GPIO_Init->Speed << (lu32_position * 2u));
      GPIOx->OSPEEDR = temp;
    }
  }
}

/*
void My_HAL_GPIO_DeInit(GPIO_TypeDef  *GPIOx, uint32_t GPIO_Pin)
{

}
*/

GPIO_PinState My_HAL_GPIO_ReadPin(GPIO_TypeDef *GPIOx, uint16_t GPIO_Pin) {
  GPIO_PinState state;
  if ((GPIOx->IDR & GPIO_Pin) == (uint32_t)0x00) {
    state = GPIO_PIN_RESET;
  } else {
    state = GPIO_PIN_SET;
  }

  return state;
}

void My_HAL_GPIO_WritePin(GPIO_TypeDef *GPIOx, uint16_t GPIO_Pin,
                          GPIO_PinState PinState) {
  if (PinState == GPIO_PIN_SET) {
    GPIOx->ODR |= GPIO_Pin;
  } else {
    GPIOx->ODR &= ~GPIO_Pin;
  }
}

void My_HAL_GPIO_TogglePin(GPIO_TypeDef *GPIOx, uint16_t GPIO_Pin) {
  GPIOx->ODR ^= GPIO_Pin;
}

void My_EXTI_CNTLR() {
  EXTI->IMR |= EXTI_IMR_MR0;
  EXTI->RTSR |= EXTI_RTSR_TR0;
  SYSCFG->EXTICR[0] &= ~(SYSCFG_EXTICR1_EXTI0);
  NVIC_EnableIRQ(EXTI0_1_IRQn);
  NVIC_SetPriority(EXTI0_1_IRQn, 1);
}
