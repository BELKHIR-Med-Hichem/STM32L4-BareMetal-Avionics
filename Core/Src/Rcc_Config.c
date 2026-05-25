/**
 * @file    Rcc_Config.c
 * @author  Meh
 * @date    Jan 20, 2026
 * @brief   Implementation of the STM32L412KB system clock configuration.
 */

#include "stm32l432xx.h"

/**
 * @brief   Configures the system clock to 80MHz using the HSI and main PLL.
 * @details The configuration executes the following strict sequence to safely
 * ramp up the core frequency:
 * * 1. Enable the 16MHz High-Speed Internal (HSI) oscillator and wait for stability.
 * 2. Adapt Flash latency to 4 Wait States (required for >64MHz operation at 3.3V)
 * and set the internal voltage regulator to Range 1 (high performance).
 * 3. Configure the Main PLL.
 * Math: (HSI / M) * N / R => (16MHz / 2) * 20 / 2 = 80MHz.
 * 4. Switch the System Clock (SYSCLK) multiplexer to the PLL output.
 */
void Sys_CLK_Config(void)
{
    /* 1 - Enable the HSI (16MHz) and wait for stabilization */
    RCC->CR |= RCC_CR_HSION;
    while (!(RCC->CR & RCC_CR_HSIRDY)); // Block until HSI is ready

    /* 2 - Adapt Flash memory latency and Voltage Scaling (VOS) for high frequency */

    /* Clear current latency settings */
    FLASH->ACR &= ~FLASH_ACR_LATENCY;

    /* Set Flash latency to 4 wait states (required for 80MHz clock) */
    FLASH->ACR |= FLASH_ACR_LATENCY_4WS;
    while ((FLASH->ACR & FLASH_ACR_LATENCY) != FLASH_ACR_LATENCY_4WS);

    /* Enable the Instruction Cache, Data Cache, and Prefetch buffer to optimize performance */
    FLASH->ACR |= FLASH_ACR_ICEN | FLASH_ACR_DCEN | FLASH_ACR_PRFTEN;

    /* Enable the Power Interface Clock */
    RCC->APB1ENR1 |= RCC_APB1ENR1_PWREN;

    /* Set Voltage Regulator to Range 1 (High Performance Mode) to support 80MHz */
    PWR->CR1 &= ~PWR_CR1_VOS;
    PWR->CR1 |= PWR_CR1_VOS_0;  // VOS = 01 (Range 1)

    /* 3 - Set the PLL multipliers and dividers */

    /* The PLL must be disabled before writing to the PLLCFGR register */
    RCC->CR &= ~RCC_CR_PLLON;
    while (RCC->CR & RCC_CR_PLLRDY); // Wait until the PLL is fully stopped

    /* Reset PLLCFGR to a known safe state */
    RCC->PLLCFGR = 0;

    /* PLL Configuration:
     * Source = HSI (16MHz)
     * M = 2  (Bitfield value is M-1 = 1)   -> VCO In = 16MHz / 2 = 8MHz
     * N = 20                               -> VCO Out = 8MHz * 20 = 160MHz
     * R = 2  (Bitfield value is 00)        -> PLLCLK = 160MHz / 2 = 80MHz
     */
    RCC->PLLCFGR |= RCC_PLLCFGR_PLLSRC_HSI;
    RCC->PLLCFGR |= (1U << RCC_PLLCFGR_PLLM_Pos);
    RCC->PLLCFGR |= (20U << RCC_PLLCFGR_PLLN_Pos);
    RCC->PLLCFGR &= ~RCC_PLLCFGR_PLLR;

    /* Enable the Main PLL Output (PLLCLK) */
    RCC->PLLCFGR |= RCC_PLLCFGR_PLLREN;

    /* Configure AHB and APB bus prescalers */
    RCC->CFGR &= ~RCC_CFGR_HPRE;   // AHB prescaler  = 1 (HCLK = 80MHz)
    RCC->CFGR &= ~RCC_CFGR_PPRE1;  // APB1 prescaler = 1 (PCLK1 = 80MHz)
    RCC->CFGR &= ~RCC_CFGR_PPRE2;  // APB2 prescaler = 1 (PCLK2 = 80MHz)

    /* Re-enable the PLL with the new configuration */
    RCC->CR |= RCC_CR_PLLON;
    while (!(RCC->CR & RCC_CR_PLLRDY)); // Wait until the PLL is locked and ready

    /* 4 - Switch the System Clock (SYSCLK) to the output of the PLL */

    /* Clear the SW (System clock switch) bits */
    RCC->CFGR &= ~RCC_CFGR_SW;

    /* Set SW bits to select PLL as the system clock source */
    RCC->CFGR |= RCC_CFGR_SW_PLL;

    /* Wait until the switch status (SWS) confirms the PLL is being used */
    while ((RCC->CFGR & RCC_CFGR_SWS) != RCC_CFGR_SWS_PLL);

    /* Update the global CMSIS variable to reflect the new 80MHz core clock */
    SystemCoreClock = 80000000U;
}
