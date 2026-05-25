/**
 * @file    ARINC429.c
 * @author  Meh
 * @date    May 9, 2026
 * @brief   Implementation of ARINC 429 initialization, encoding, and transmission.
 */

#include "ARINC429.h"
#include "STM32l432xx.h"
#include "delay.h"

/**
 * @brief   Initializes the GPIO pins required for ARINC 429 transmission.
 * @details Configures PA1 and PA8 as general purpose output, push-pull,
 * with very high output speed to handle the sharp 100kbps pulse edges.
 */
void ARINC429_Init(void)
{
    /* Enable the clock for GPIOA */
    RCC->AHB2ENR |= RCC_AHB2ENR_GPIOAEN;

    /* Clear MODER bits for PA1 and PA8 */
    GPIOA->MODER &= ~(3U << (8 * 2));
    GPIOA->MODER &= ~(3U << (1 * 2));

    /* Set PA1 and PA8 to General Purpose Output mode (01) */
    GPIOA->MODER |= (1U << (8 * 2));
    GPIOA->MODER |= (1U << (1 * 2));

    /* Set Output Type to Push-Pull (clear OTYPER bits) */
    GPIOA->OTYPER &= ~(1U << 8);
    GPIOA->OTYPER &= ~(1U << 1);

    /* Set Output Speed to Very High (11) for sharp RZ signal edges */
    GPIOA->OSPEEDR |= (3U << (8 * 2));
    GPIOA->OSPEEDR |= (3U << (1 * 2));
}

/**
 * @brief   Encodes a floating-point value into a 32-bit ARINC 429 word.
 */
uint32_t ARINC429_Encode(float data)
{
    uint32_t word = 0;

    /* Label: 256 in octal is 0xAE in HEX. ARINC requires the label to be
       transmitted bit-reversed, so 0xAE becomes 0x75. */
    uint32_t label = 0x75;

    /* Source/Destination Identifier (SDI) */
    uint32_t SDI = 0;

    /* Sign/Status Matrix (SSM) */
    uint32_t SSM = 0;

    /* Convert float data to Binary Number Representation (BNR).
       Scale factor = (Value / Max_Range) * (2^(Bits-1))
       For 19 bits, the max signed value is 2^18 = 262144.
       Assumed Max_Range is 2048.0f. */
    int32_t bnr_data = (int32_t)((data / 2048.0f) * 262144.0f);

    /* Mask the data to ensure it strictly occupies only 19 bits */
    uint32_t data_429 = bnr_data & 0x7FFFF;

    /* Assemble the word: Label (8) + SDI (2) + Data (19) + SSM (2) */
    word |= (label << 0) | (SDI << 8) | (data_429 << 10) | (SSM << 29);

    /* Calculate Odd Parity for bit 32 */
    uint32_t parity_calc = word;
    uint32_t bit_count = 0;

    /* Count the number of set bits (1s) in the 31-bit word */
    while (parity_calc) {
        bit_count ^= (parity_calc & 1);
        parity_calc >>= 1;
    }

    /* If bit_count is 0 (meaning we have an even number of 1s),
       set the 32nd bit to 1 to achieve ODD parity. */
    if (bit_count == 0) {
        word |= (1U << 31);
    }

    return word;
}

/**
 * @brief   Transmits a 32-bit ARINC 429 word via GPIO bit-banging.
 */
void ARINC429_Transmit(uint32_t arinc_word)
{
    /* Transmit all 32 bits, LSB (Bit 1) first */
    for (int i = 0; i < 32; i++) {
        uint8_t bit = (arinc_word >> i) & 1;

        if (bit == 1) {
            /* Transmit '1' (HIGH state)
               Set PA1 (TXA High), Reset PA8 (TXB Low) */
            GPIOA->BSRR = (1U << 1) | (1U << 24);
        } else {
            /* Transmit '0' (LOW state)
               Reset PA1 (TXA Low), Set PA8 (TXB High) */
            GPIOA->BSRR = (1U << 17) | (1U << 8);
        }

        /* Hold the active pulse for 5µs (half the bit time for 100kbps) */
        delay_us(5);

        /* Return to Zero (NULL state)
           Reset both PA1 and PA8 */
        GPIOA->BSRR = (1U << 17) | (1U << 24);

        /* Hold the NULL state for the remaining 5µs of the bit time */
        delay_us(5);
    }

    /* 4-bit minimum inter-word gap required by ARINC 429 standard.
       At 100kbps, 1 bit = 10µs, so 4 bits = 40µs gap. */
    delay_us(40);
}
