/**
 * @file    Rcc_Config.h
 * @author  Meh
 * @date    Jan 20, 2026
 * @brief   System clock configuration for the STM32L412KB.
 * @details Provides the function prototype to initialize the core system clock
 * to its maximum frequency of 80MHz using the internal HSI oscillator
 * and the Phase-Locked Loop (PLL).
 */

#ifndef INC_RCC_CONFIG_H_
#define INC_RCC_CONFIG_H_

#include "stm32l432xx.h"

/**
 * @brief   Configures the system clock to 80MHz using the HSI and main PLL.
 * @details Executes the following hardware sequence:
 * -# Turns on the 16MHz High-Speed Internal (HSI) oscillator.
 * -# Configures Flash memory latency (4 wait states) and sets Voltage Range 1.
 * -# Configures the PLL multipliers and dividers to output 80MHz.
 * -# Switches the System Clock (SYSCLK) source to the PLL output.
 * @return  None
 * @note    This function MUST be called at the very beginning of main(),
 * before initializing any other peripherals or delays.
 */
void Sys_CLK_Config(void);

#endif /* INC_RCC_CONFIG_H_ */
