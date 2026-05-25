/**
 * @file    ARINC429.h
 * @author  Meh
 * @date    May 6, 2026
 * @brief   Header file for ARINC 429 protocol encoding and transmission.
 * @details This module provides functions to initialize the hardware, encode
 * floating-point data into standard 32-bit ARINC 429 words (BNR format),
 * and transmit them using bit-banging with a Return-to-Zero (RZ) scheme.
 */

#ifndef INC_ARINC429_H_
#define INC_ARINC429_H_

#include <stdint.h>

/**
 * @brief   Initializes the GPIO pins required for ARINC 429 transmission.
 * @details Configures GPIOA Pin 1 and Pin 8 as high-speed push-pull outputs.
 * These two pins act as the differential lines (TXA and TXB) for the
 * ARINC bus.
 * @return  None
 */
void ARINC429_Init(void);

/**
 * @brief   Encodes a floating-point value into a 32-bit ARINC 429 word.
 * @details Uses Binary Number Representation (BNR) format. The word structure:
 * - Bits 1-8:   Label (Octal 256 -> 0xAE, bit-reversed to 0x75)
 * - Bits 9-10:  Source/Destination Identifier (SDI = 0)
 * - Bits 11-29: 19-bit signed data (Max range: 2048.0)
 * - Bits 30-31: Sign/Status Matrix (SSM = 0)
 * - Bit 32:     Odd Parity bit
 * @param   data The floating-point value to encode.
 * @return  uint32_t The fully assembled 32-bit ARINC 429 word.
 */
uint32_t ARINC429_Encode(float data);

/**
 * @brief   Transmits a 32-bit ARINC 429 word via GPIO bit-banging.
 * @details Operates at 100 kbps (High-Speed ARINC). Each bit takes 10µs total:
 * - 5µs active pulse (High or Low)
 * - 5µs Return-to-Zero (NULL state)
 * After all 32 bits are sent (LSB first), a 40µs gap (4 bit times)
 * is added to satisfy the ARINC 429 inter-word gap requirement.
 * @param   arinc_word The 32-bit data word to transmit.
 * @return  None
 */
void ARINC429_Transmit(uint32_t arinc_word);

#endif /* INC_ARINC429_H_ */
