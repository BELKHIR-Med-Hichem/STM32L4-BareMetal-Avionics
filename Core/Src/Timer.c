/**
 * @file    Timer.c
 * @author  Meh
 * @date    May 9, 2026
 * @brief   Implementation of the Hardware Timer driver.
 */

#include "Rcc_Config.h"
#include "Timer.h"

/**
 * @brief   Initializes Timer 2 (TIM2) for PWM signal generation on PA0.
 */
void PWM_Init(void)
{
    /* 1 - Enable peripheral clocks for TIM2 (APB1) and GPIOA (AHB2) */
    RCC->APB1ENR1 |= 0x01U;
    RCC->AHB2ENR  |= RCC_AHB2ENR_GPIOAEN;

    /* 2 - GPIO Configuration for PA0 */
    GPIOA->MODER   &= ~(3U << (0 * 2)); /* Clear mode bits for PA0 */
    GPIOA->MODER   |=  (2U << (0 * 2)); /* Set PA0 to Alternate Function mode (10) */
    GPIOA->OTYPER  &= ~(1U << 0);       /* Set PA0 output type to Push-Pull */
    GPIOA->OSPEEDR |=  (3U << (0 * 2)); /* Set PA0 to Very High speed */
    GPIOA->AFR[0]  |=  (1U << (0 * 4)); /* Assign Alternate Function 1 (AF1 = TIM2_CH1) to PA0 */

    /* 3 - Timer 2 Core Configuration */
    TIM2->CR1   |= 0x80U; /* Enable Auto-reload preload (ARPE) */
    TIM2->CCMR1 |= 0x68U; /* Output Compare 1 Preload enable + PWM Mode 1 (110) */
    TIM2->CCER  |= 0x01U; /* Enable Capture/Compare 1 output (CC1E) on the pin */

    /* 4 - Timer 2 Timing Parameters (Target: 50Hz / 20ms period) */
    TIM2->CNT  = 0U;          /* Reset the counter */

    /* Prescaler: Divide 80MHz system clock by 80 to get a 1MHz timer clock (1 tick = 1µs) */
    TIM2->PSC  = 80U - 1U;

    /* Auto-Reload Register: 20000 ticks @ 1MHz = 20ms period (50Hz) */
    TIM2->ARR  = 20000U - 1U;

    /* Capture/Compare Register 1: 1500 ticks @ 1MHz = 1.5ms pulse width (Standard Servo Neutral) */
    TIM2->CCR1 = 1500U;

    /* 5 - Start the Timer */
    TIM2->EGR |= 0x01U; /* Generate Update Event (UG) to immediately load shadow registers */
    TIM2->CR1 |= 0x01U; /* Enable TIM2 counter (CEN) */
}

/**
 * @brief   Initializes Timer 6 (TIM6) as a periodic interrupt timer for sensors.
 */
void Sensor_Timer(void)
{
    /* 1 - Enable clock for basic timer TIM6 on APB1 */
    RCC->APB1ENR1 |= 0x10U;

    /* Enable Auto-reload preload (ARPE) */
    TIM6->CR1 |= 0x80U;

    /* 2 - Timer 6 Timing Parameters (Target: 50Hz / 20ms period) */
    TIM6->CNT = 0U;

    /* Prescaler: Divide 80MHz system clock by 80 to get a 1MHz timer clock (1 tick = 1µs) */
    TIM6->PSC = 80U - 1U;

    /* Auto-Reload Register: 20000 ticks @ 1MHz = 20ms period (50Hz) */
    TIM6->ARR = 20000U - 1U;

    /* 3 - Interrupt Configuration */
    TIM6->EGR  |= 0x01U;			/* Generate Update Event to load shadow registers */
    TIM6->SR   &= ~0x01U;			/* Clear the Update Interrupt Flag (UIF) to prevent immediate false trigger */
    TIM6->DIER |= 0x01U;			/* Enable Update Interrupt (UIE) */

    /* Enable TIM6 interrupt in the Nested Vectored Interrupt Controller (NVIC) */
    NVIC_EnableIRQ(TIM6_DAC_IRQn);

    /* 4 - Start the Timer */
    TIM6->CR1 |= 0x01U;			    /* Enable TIM6 counter (CEN) */
}

/**
 * @brief   Initializes Timer 7 (TIM7) as a periodic interrupt timer for the display.
 */
void Display_Timer(void)
{
    /* 1 - Enable clock for basic timer TIM7 on APB1 */
    RCC->APB1ENR1 |= 0x20U;

    /* Enable Auto-reload preload (ARPE) */
    TIM7->CR1 |= 0x80U;

    /* 2 - Timer 7 Timing Parameters (Target: 10Hz / 100ms period) */
    TIM7->CNT = 0U;

    /* Prescaler: Divide 80MHz system clock by 800 to get a 100kHz timer clock (1 tick = 10µs) */
    TIM7->PSC = 800U - 1U;

    /* Auto-Reload Register: 10000 ticks @ 100kHz = 100ms period (10Hz) */
    TIM7->ARR = 10000U - 1U;

    /* 3 - Interrupt Configuration */
    TIM7->EGR  |= 0x01U;			/* Generate Update Event to load shadow registers */
    TIM7->SR   &= ~0x01U;			/* Clear the Update Interrupt Flag (UIF) to prevent immediate false trigger */
    TIM7->DIER |= 0x01U;			/* Enable Update Interrupt (UIE) */

    /* Enable TIM7 interrupt in the Nested Vectored Interrupt Controller (NVIC) */
    NVIC_EnableIRQ(TIM7_IRQn);

    /* 4 - Start the Timer */
    TIM7->CR1 |= 0x01U;			    /* Enable TIM7 counter (CEN) */
}
