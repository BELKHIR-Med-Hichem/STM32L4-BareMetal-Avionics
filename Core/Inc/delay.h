/**
 * @file    delay.h
 * @author  Meh
 * @date    Jan 20, 2026
 * @brief   Microsecond and millisecond busy-wait delay using the ARM DWT cycle counter.
 * @details This module uses the Cortex-M4 Data Watchpoint and Trace (DWT) unit's
 * free-running cycle counter (CYCCNT) to implement precise busy-wait delays
 * without consuming a hardware timer peripheral.
 * * Available macros:
 * - delay_us(n) : waits for n microseconds.
 * - delay_ms(n) : waits for n milliseconds.
 * * @note    DWT_Init() must be called once before any delay function or macro is used.
 */

#ifndef INC_DELAY_H_
#define INC_DELAY_H_

#include "stdint.h"
#include "stm32l4xx.h"

/**
 * @brief   Busy-wait delay for the given number of microseconds.
 * @param   n Number of microseconds to delay.
 * @note    DWT_Init() must be called before use.
 */
#define delay_us(n) DWT_Delay(n)

/**
 * @brief   Busy-wait delay for the given number of milliseconds.
 * @param   n Number of milliseconds to delay.
 * @note    DWT_Init() must be called before use.
 */
#define delay_ms(n) DWT_Delay((n) * 1000U)

/**
 * @brief   Enables the DWT cycle counter (CYCCNT).
 * @details Enables the DWT trace unit via the CoreDebug DEMCR register, resets
 * the CYCCNT counter to zero, then starts the counter. It also pre-calculates
 * the multiplier required to convert microseconds to CPU cycles.
 * @note    Must be called once at startup before any call to DWT_Delay(),
 * delay_us(), or delay_ms().
 * @return  None
 */
void DWT_Init(void);

/**
 * @brief   Global multiplier holding the number of CPU cycles per microsecond.
 * @details Calculated once in DWT_Init() as (SystemCoreClock / 1000000).
 */
extern uint32_t dWT_delay_multiplier;

/**
 * @brief   Inline function to execute a precise microsecond busy-wait.
 * @details Calculates the target number of CPU cycles required for the delay.
 * It subtracts a predefined overhead constant to account for function call
 * and loop setup times. It uses the `__attribute__((always_inline))`
 * directive to minimize branching overhead.
 * @param   us The number of microseconds to delay.
 * @return  None
 */
static inline void __attribute__((always_inline)) DWT_Delay(uint32_t us)
{
    uint32_t startTick = DWT->CYCCNT;
    uint32_t delayTicks = us * dWT_delay_multiplier;

    /* The number of cycles it takes to set up the loop and exit.
       Tweak this number (e.g., 10, 15, 20) based on oscilloscope measurements
       to achieve exactly 1.0 us for small delays! */
    const uint32_t OVERHEAD_CYCLES = 10;

    /* Subtract overhead to maintain accuracy, guarding against integer underflow */
    if (delayTicks > OVERHEAD_CYCLES) {
        delayTicks -= OVERHEAD_CYCLES;
    } else {
        delayTicks = 0; // Prevent underflow for extremely tiny delays
    }

    /* Busy-wait until the elapsed cycles meet or exceed the target ticks */
    while ((DWT->CYCCNT - startTick) < delayTicks);
}

#endif /* INC_DELAY_H_ */
