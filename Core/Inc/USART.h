/**
 * @file    USART.h
 * @author  Meh
 * @date    Jan 21, 2026
 * @brief   Bare-metal USART2 driver for the STM32L412.
 * @details This module configures USART2 on PA2 (TX) and PA15 (RX) at 115200 baud,
 * 8-bit data, no parity, 1 stop bit (8N1), operating on an 80MHz PCLK.
 * It provides a simple blocking transmission interface and implements the
 * `_write()` syscall redirect, allowing standard C library functions like `printf()`
 * to output seamlessly over the serial port.
 * @note    Sys_CLK_Config() must be called before USART2_Config().
 */

#ifndef INC_USART_H_
#define INC_USART_H_

#include "Rcc_Config.h"
#include "stdint.h"
#include <string.h>
#include <stdarg.h>

/**
 * @brief   Syscall retarget — redirects printf() and other stdio output to USART2.
 * @details This function overrides the weak _write() stub in the newlib syscalls.
 * It is called automatically by printf(), puts(), and similar standard library functions.
 * Transmission is fully blocking: it polls the TXE flag for each byte.
 * @param   file File descriptor (ignored — all output is routed directly to USART2).
 * @param   ptr  Pointer to the buffer of characters to transmit.
 * @param   len  Number of bytes to transmit.
 * @return  int  Number of bytes written (always equal to len).
 */
int _write(int file, char *ptr, int len);

/**
 * @brief   Configures USART2 at 115200 baud, 8N1, on PA2 (TX) and PA15 (RX).
 * @details The initialization executes the following hardware sequence:
 * -# Enables the clock for GPIOA (AHB2) and USART2 (APB1).
 * -# Sets PA2 and PA15 to Alternate Function mode.
 * -# Maps Alternate Function 7 (AF7) to PA2 for TX, and AF3 to PA15 for RX.
 * -# Configures the Baud Rate Register (BRR) for 115200 at 80MHz PCLK (BRR = 694).
 * -# Enables the transmitter (TE), receiver (RE), and the main USART peripheral (UE).
 * @return  None
 */
void USART2_Config(void);

/**
 * @brief   Transmits a null-terminated string over USART2 in blocking mode.
 * @details Iterates through the provided string, polling the Transmit Data Register
 * Empty (TXE) flag before loading each subsequent character into the TDR.
 * @param   str Pointer to the null-terminated string to transmit.
 * @return  None
 */
void USART2_SendString(char *str);

#endif /* INC_USART_H_ */
