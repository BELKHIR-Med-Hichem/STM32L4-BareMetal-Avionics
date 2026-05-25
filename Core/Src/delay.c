/**
 * @file    delay.c
 * @author  Meh
 * @date    Jan 20, 2026
 * @brief   Implementation of the DWT-based delay initialization.
 */

#include "delay.h"

/**
 * @brief Global variable initialized to 0, holding cycles per microsecond.
 */
uint32_t dWT_delay_multiplier = 0;

/**
 * @brief   Enables the DWT cycle counter (CYCCNT).
 * @details Checks if the trace unit is enabled. If not, it sets the TRCENA bit
 * in the DEMCR register, clears the cycle counter, and enables it.
 * Finally, it fetches the current SystemCoreClock to establish the
 * us-to-cycles multiplier.
 */
void DWT_Init(void)
{
    /* Check if the Trace enable bit is set in the CoreDebug register */
    if (!(CoreDebug->DEMCR & CoreDebug_DEMCR_TRCENA_Msk)) {

        /* Enable TRCENA */
        CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;

        /* Reset the Cycle Counter */
        DWT->CYCCNT = 0;

        /* Enable the Cycle Counter */
        DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
    }

    /* Calculate the multiplier ONCE during initialization to save CPU time later.
       (SystemCoreClock / 1000000) gives the exact number of ticks per 1 us. */
    dWT_delay_multiplier = SystemCoreClock / 1000000U;
}
