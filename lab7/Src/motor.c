#include <stdio.h>
#include <stdlib.h>
#include <stm32f0xx.h>
#include <SEGGER_RTT.h>
#include "motor.h"

extern void Lab7_Systick_Callback(void);
/* -------------------------------------------------------------------------------------------------------------
 *  Global Variable and Type Declarations
 *  -------------------------------------------------------------------------------------------------------------
 */

volatile int16_t error_integral;    // Integrated error signal
volatile uint8_t duty_cycle;    // Output PWM duty cycle
volatile int16_t target_rpm;    // Desired speed target
volatile int16_t motor_speed;   // Measured motor speed
volatile int8_t adc_value;      // ADC measured motor current
volatile int16_t error;         // Speed error signal
volatile uint8_t Kp;            // Proportional gain
volatile uint8_t Ki;            // Integral gain

static uint8_t buf0[1024];
static uint8_t buf1[1024];
static uint8_t buf2[1024];

union byte_split {
    uint32_t uword;
    int32_t word;
    uint8_t bytes[4];
};

void log_init(void) {
    SEGGER_RTT_ConfigUpBuffer(0, "", buf0, 1024, SEGGER_RTT_MODE_NO_BLOCK_SKIP);
    SEGGER_RTT_ConfigUpBuffer(1, "", buf1, 1024, SEGGER_RTT_MODE_NO_BLOCK_SKIP);
    SEGGER_RTT_ConfigUpBuffer(2, "", buf2, 1024, SEGGER_RTT_MODE_NO_BLOCK_SKIP);
}

void Lab7_SysTick_Handler(void) {
    Lab7_Systick_Callback();  // defined in lab7.c
    log_data();
}

void log_data(void) {
    __disable_irq();
    uint32_t duty_cycle_copy = duty_cycle;
    int32_t target_rpm_copy = target_rpm;
    int32_t motor_speed_copy = motor_speed;
    __enable_irq();

    union byte_split data;
    data.uword = duty_cycle_copy;
    SEGGER_RTT_Write (0, &data.bytes, 4);
    data.word = target_rpm_copy;
    SEGGER_RTT_Write (1, &data.bytes, 4);
    data.word = motor_speed_copy;
    SEGGER_RTT_Write (2, &data.bytes, 4);
}
// Sets up the entire motor drive system
void motor_init(void) {
    log_init();
    pwm_init();
    encoder_init();
    ADC_init();
}

// Sets up the PWM and direction signals to drive the H-Bridge
void pwm_init(void) {

    // Set up pin PA4 for H-bridge PWM output (TIMER 14 CH1)
    GPIOA->MODER |= (1 << 9);
    GPIOA->MODER &= ~(1 << 8);

    // Set PA4 to AF4,
    GPIOA->AFR[0] &= 0xFFF0FFFF; // clear PA4 bits,
    GPIOA->AFR[0] |= (1 << 18);

    // Set up a PA5, PA6 as GPIO output pins for motor direction control
    GPIOA->MODER &= 0xFFFFC3FF; // clear PA5, PA6 bits,
    GPIOA->MODER |= (1 << 10) | (1 << 12);

    //Initialize one direction pin to high, the other low
    GPIOA->ODR |= (1 << 5);
    GPIOA->ODR &= ~(1 << 6);

    // Set up PWM timer
    RCC->APB1ENR |= RCC_APB1ENR_TIM14EN;
    TIM14->CR1 = 0;                         // Clear control registers
    TIM14->CCMR1 = 0;                       // (prevents having to manually clear bits)
    TIM14->CCER = 0;

    // Set output-compare CH1 to PWM1 mode and enable CCR1 preload buffer
    TIM14->CCMR1 |= (TIM_CCMR1_OC1M_2 | TIM_CCMR1_OC1M_1 | TIM_CCMR1_OC1PE);
    TIM14->CCER |= TIM_CCER_CC1E;           // Enable capture-compare channel 1
    TIM14->PSC = 1;                         // Run timer on 24Mhz
    TIM14->ARR = 1200;                      // PWM at 20kHz
    TIM14->CCR1 = 0;                        // Start PWM at 0% duty cycle

    TIM14->CR1 |= TIM_CR1_CEN;              // Enable timer
}

// Set the duty cycle of the PWM, accepts (0-100)
void pwm_setDutyCycle(uint8_t duty) {
    if(duty <= 100) {
        TIM14->CCR1 = ((uint32_t)duty*TIM14->ARR)/100;  // Use linear transform to produce CCR1 value
        // (CCR1 == "pulse" parameter in PWM struct used by peripheral library)
    }
}

// Sets up encoder interface to read motor speed
void encoder_init(void) {

    // Set up encoder input pins (TIMER 3 CH1 and CH2)
    RCC->AHBENR |= RCC_AHBENR_GPIOBEN;

    GPIOB->MODER &= ~(GPIO_MODER_MODER4_0 | GPIO_MODER_MODER5_0);
    GPIOB->MODER |= (GPIO_MODER_MODER4_1 | GPIO_MODER_MODER5_1);
    GPIOB->AFR[0] |= ( (1 << 16) | (1 << 20) );

    // Set up encoder interface (TIM3 encoder input mode)
    RCC->APB1ENR |= RCC_APB1ENR_TIM3EN;
    TIM3->CCMR1 = 0;
    TIM3->CCER = 0;
    TIM3->SMCR = 0;
    TIM3->CR1 = 0;

    TIM3->CCMR1 |= (TIM_CCMR1_CC1S_0 | TIM_CCMR1_CC2S_0);   // TI1FP1 and TI2FP2 signals connected to CH1 and CH2
    TIM3->SMCR |= (TIM_SMCR_SMS_1 | TIM_SMCR_SMS_0);        // Capture encoder on both rising and falling edges
    TIM3->ARR = 0xFFFF;                                     // Set ARR to top of timer (longest possible period)
    TIM3->CNT = 0x7FFF;                                     // Bias at midpoint to allow for negative rotation
    // (Could also cast unsigned register to signed number to get negative numbers if it rotates backwards past zero
    //  just another option, the mid-bias is a bit simpler to understand though.)
    TIM3->CR1 |= TIM_CR1_CEN;                               // Enable timer

    // Configure a second timer (TIM6) to fire an ISR on update event
    // Used to periodically check and update speed variable
    RCC->APB1ENR |= RCC_APB1ENR_TIM6EN;

    // Select PSC and ARR values that give an appropriate interrupt rate
    TIM6->PSC = 11;
    TIM6->ARR = 30000;

    TIM6->DIER |= TIM_DIER_UIE;             // Enable update event interrupt
    TIM6->CR1 |= TIM_CR1_CEN;               // Enable Timer

    NVIC_EnableIRQ(TIM6_DAC_IRQn);          // Enable interrupt in NVIC
    NVIC_SetPriority(TIM6_DAC_IRQn,2);
}

// Encoder interrupt to calculate motor speed, also manages PI controller
void TIM6_DAC_IRQHandler(void) {
    /* Calculate the motor speed in raw encoder counts
     * Note the motor speed is signed! Motor can be run in reverse.
     * Speed is measured by how far the counter moved from center point
     */
    motor_speed = (TIM3->CNT - 0x7FFF);
    TIM3->CNT = 0x7FFF; // Reset back to center point

    // Call the PI update function
    PI_update();

    TIM6->SR &= ~TIM_SR_UIF;        // Acknowledge the interrupt
}

void ADC_init(void) {

    // Configure PA1 for ADC input (used for current monitoring)
    GPIOA->MODER |= (GPIO_MODER_MODER1_0 | GPIO_MODER_MODER1_1);

    // Configure ADC to 8-bit continuous-run mode, (asynchronous clock mode)
    RCC->APB2ENR |= RCC_APB2ENR_ADCEN;

    ADC1->CFGR1 = 0;                        // Default resolution is 12-bit (RES[1:0] = 00 --> 12-bit)
    ADC1->CFGR1 |= ADC_CFGR1_CONT;          // Set to continuous mode
    ADC1->CHSELR |= ADC_CHSELR_CHSEL1;      // Enable channel 1

    ADC1->CR = 0;
    ADC1->CR |= ADC_CR_ADCAL;               // Perform self calibration
    while(ADC1->CR & ADC_CR_ADCAL);         // Delay until calibration is complete

    ADC1->CR |= ADC_CR_ADEN;                // Enable ADC
    while(!(ADC1->ISR & ADC_ISR_ADRDY));    // Wait until ADC ready
    ADC1->CR |= ADC_CR_ADSTART;             // Signal conversion start
}

/* Run PI control loop
 *
 * Make sure to use the indicated variable names. This allows STMStudio to monitor
 * the condition of the system!
 *
 * target_rpm -> target motor speed in RPM
 * motor_speed -> raw motor speed in encoder counts
 * error -> error signal (difference between measured speed and target)
 * error_integral -> integrated error signal
 * Kp -> Proportional Gain
 * Ki -> Integral Gain
 * output -> raw output signal from PI controller
 * duty_cycle -> used to report the duty cycle of the system
 * adc_value -> raw /disaADC counts to report current
 *
 */
void PI_update(void) {
    __disable_irq();


    // At 45ms period: counts = RPM * 3200 * 0.045 / 60 = RPM * 12 / 5
    int16_t target_counts = (int16_t)(target_rpm * 12 / 5);

    // Calculate error (difference between target and measured speed)
    error = target_counts - motor_speed;

    // Calculate integral: y[n] = y[n-1] + (Ki * error)
    error_integral = error_integral + (Ki * error);

    // Clamp integral to prevent windup (0 to 3200)
    if (error_integral > 3200) {
        error_integral = 3200;
    } else if (error_integral < 0) {
        error_integral = 0;
    }

    // Calculate PI output: proportional + integral
    int16_t output = (Kp * error) + error_integral;

    // Divide output into duty cycle range (divide by 32 = right shift 5)
    output = output >> 5;

    // Clamp output between 0 and 100
    if (output > 100) {
        output = 100;
    } else if (output < 0) {
        output = 0;
    }

    pwm_setDutyCycle(output);
    duty_cycle = output;

    if(ADC1->ISR & ADC_ISR_EOC) {
        adc_value = ADC1->DR;
    }

    __enable_irq();



    

    
}