/**
 * @file    USART.c
 * @author  Meh
 * @date    Jan 21, 2026
 * @brief   Implementation of the USART2 bare-metal driver and stdio retargeting.
 */

#include "USART.h"

/**
 * @brief   Syscall retarget — redirects printf() and other stdio output to USART2.
 */
int _write(int file, char *ptr, int len)
{
    for (int i = 0; i < len; i++)
    {
        /* Wait until the Transmit Data Register is empty (TXE flag set) */
        while (!(USART2->ISR & USART_ISR_TXE));

        /* Load the next character into the Transmit Data Register */
        USART2->TDR = ptr[i];
    }
    return len;
}

/**
 * @brief   Configures USART2 at 115200 baud, 8N1, on PA2 (TX) and PA15 (RX).
 */
void USART2_Config(void)
{
    /* 1 - Enable the USART2 clock on APB1 and GPIOA clock on AHB2 */
    RCC->APB1ENR1 |= RCC_APB1ENR1_USART2EN;
    RCC->AHB2ENR  |= RCC_AHB2ENR_GPIOAEN;

    /* 2 - Set PA2 (TX) and PA15 (RX) to Alternate Function mode (10) */
    GPIOA->MODER &= ~(3U << (2 * 2));   // Clear PA2 mode bits
    GPIOA->MODER |=  (2U << (2 * 2));   // Set PA2 to AF
    GPIOA->MODER &= ~(3U << (15 * 2));  // Clear PA15 mode bits
    GPIOA->MODER |=  (2U << (15 * 2));  // Set PA15 to AF

    /* 3 - Set PA2 and PA15 to High Speed to ensure crisp signal edges */
    GPIOA->OSPEEDR |= (3U << (2 * 2)) | (3U << (15 * 2));

    /* 4 - Configure the specific Alternate Functions for the pins */
    /* Alternate Function 7 (AF7) for PA2 (USART2_TX) */
    GPIOA->AFR[0] &= ~(0xF << (2 * 4));
    GPIOA->AFR[0] |=  (7U << (2 * 4));

    /* Alternate Function 3 (AF3) for PA15 (USART2_RX) */
    GPIOA->AFR[1] &= ~(0xF << ((15 - 8) * 4));
    GPIOA->AFR[1] |=  (3U << ((15 - 8) * 4));

    /* 5 - Configure USART2 Control Registers */
    /* Ensure USART is disabled before applying configuration */
    USART2->CR1 = 0x00;

    /* Enable the USART Peripheral (UE) */
    USART2->CR1 |= USART_CR1_UE;

    /* Set word length to 8 data bits (M bit = 0) */
    USART2->CR1 &= ~USART_CR1_M;

    /* Set the baud rate to 115200.
     * Formula: BRR = SystemCoreClock / BaudRate
     * Calculation: 80,000,000 / 115200 = 694.44 -> 694
     */
    USART2->BRR = 694;

    /* Enable Transmitter (TE), Receiver (RE), and Peripheral (UE) */
    USART2->CR1 = USART_CR1_TE | USART_CR1_RE | USART_CR1_UE;
}

/**
 * @brief   Transmits a null-terminated string over USART2 in blocking mode.
 */
void USART2_SendString(char *str)
{
    /* Iterate through the string until the null-terminator (\0) is hit */
    while (*str)
    {
        /* Wait until the Transmit Data Register is empty (TXE flag set) */
        while (!(USART2->ISR & USART_ISR_TXE));

        /* Load the next character into the Transmit Data Register and advance pointer */
        USART2->TDR = *str++;
    }
}
