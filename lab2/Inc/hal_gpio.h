#ifndef HAL_GPIO_H
#define HAL_GPIO_H

#include <stdint.h>
#include <stm32f0xx_hal.h>
#include <stm32f0xx_hal_gpio.h>

#define LED_RED 6    // PC6
#define LED_BLUE 7   // PC7
#define LED_ORANGE 8 // PC8
#define LED_GREEN 9  // PC9
#define BUTTON 0     // PA0

/* Function Prototypes */
void BSP_GPIO_Init(GPIO_TypeDef *port, uint32_t pin, uint32_t mode,
                   uint32_t speed, uint32_t pull);
uint32_t BSP_GPIO_ReadPin(GPIO_TypeDef *port, uint32_t pin);
void BSP_GPIO_WritePin(GPIO_TypeDef *port, uint32_t pin, uint32_t state);
void BSP_GPIO_TogglePin(GPIO_TypeDef *port, uint32_t pin);
void BSP_GPIO_EnableClock(GPIO_TypeDef *port);

/* EXTI Functions  */
void BSP_EXTI_Init(GPIO_TypeDef *port, uint32_t pin, uint32_t rising_edge,
                   uint32_t falling_edge);
void BSP_EXTI_EnableIRQ(uint32_t pin, uint32_t priority);
void BSP_EXTI_ClearFlag(uint32_t pin);

#endif /* HAL_GPIO_H */