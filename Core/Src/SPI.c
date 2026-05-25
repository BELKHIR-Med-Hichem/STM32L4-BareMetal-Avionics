/**
 * @file    SPI.c
 * @author  Meh
 * @date    Mar 2, 2026
 * @brief   Implementation of the bare-metal SPI1 driver.
 */

#include "SPI.h"
#include "stm32l432xx.h"

/**
 * @brief   Initializes the SPI1 peripheral in Master mode.
 */
void SPI_Init(void)
{
    /* 1 - Enable GPIOB and SPI1 clocks on their respective buses */
    RCC->AHB2ENR |= RCC_AHB2ENR_GPIOBEN;
    RCC->APB2ENR |= RCC_APB2ENR_SPI1EN;

    /* 2 - Configure Pins: PB3 (SCK), PB4 (MISO), PB5 (MOSI) to Alternate Function mode (10) */
    GPIOB->MODER &= ~((3U << (3 * 2)) | (3U << (4 * 2)) | (3U << (5 * 2)));
    GPIOB->MODER |=  ((2U << (3 * 2)) | (2U << (4 * 2)) | (2U << (5 * 2)));

    /* 3 - Map the Alternate Function to AF5 (SPI1) for PB3, PB4, and PB5 */
    GPIOB->AFR[0] &= ~((0xFU << (3 * 4)) | (0xFU << (4 * 4)) | (0xFU << (5 * 4)));
    GPIOB->AFR[0] |=  ((5U  << (3 * 4)) | (5U  << (4 * 4)) | (5U  << (5 * 4)));

    /* 4 - Set High Speed output for the clock (SCK) and data output (MOSI) pins */
    GPIOB->OSPEEDR |= (3U << (3 * 2)) | (3U << (5 * 2));

    /* 5 - Enable internal Pull-up on MISO (PB4) to stabilize the floating line */
    GPIOB->PUPDR &= ~(3U << (4 * 2));
    GPIOB->PUPDR |=  (1U << (4 * 2));

    /* 6 - SPI1 Control Register 1 Configuration */
    SPI1->CR1 = 0;
    SPI1->CR1 |= SPI_CR1_MSTR;  // Master configuration
    SPI1->CR1 |= SPI_CR1_SSM;   // Software Slave Management enabled
    SPI1->CR1 |= SPI_CR1_SSI;   // Internal Slave Select forces high (Master mode active)

    /* Set Baud Rate to fPCLK/256 (BR = 111).
       At 80MHz PCLK, this yields ~312.5 kHz (Required for SD Card initial setup) */
    SPI1->CR1 |= (7U << SPI_CR1_BR_Pos);

    /* 7 - SPI1 Control Register 2 Configuration */
    SPI1->CR2 = 0;

    /* Set Data Size (DS) to 8-bit (0111) */
    SPI1->CR2 |= (7U << SPI_CR2_DS_Pos);

    /* Set FIFO Reception Threshold to 8-bit.
       Forces RXNE event when 8 bits (1 byte) are received, instead of waiting for 16. */
    SPI1->CR2 |= SPI_CR2_FRXTH;

    /* 8 - Enable SPI1 Peripheral */
    SPI1->CR1 |= SPI_CR1_SPE;
}

/**
 * @brief   Transmits a single byte over SPI.
 */
void SPI_SendByte(uint8_t data)
{
    /* Call transfer but ignore the return value.
       Crucially, reading the DR inside TransferByte clears the RXNE flag,
       preventing overrun (OVR) errors during rapid successive writes. */
    SPI_TransferByte(data);
}

/**
 * @brief   Reads a single byte over SPI.
 */
uint8_t SPI_ReadByte(uint8_t data)
{
    /* Transmit the dummy byte to generate the clock pulses, and return what comes back */
    return SPI_TransferByte(data);
}

/**
 * @brief   Transmits and receives a byte simultaneously (Full-Duplex).
 */
uint8_t SPI_TransferByte(uint8_t data)
{
    /* Wait until the Transmit Buffer Empty (TXE) flag is set */
    while (!(SPI1->SR & SPI_SR_TXE));

    /* * STM32L4 FIFO Quirk Fix:
     * We MUST cast the Data Register pointer to a volatile 8-bit pointer.
     * If we do a standard 16-bit register write, the hardware packs the
     * FIFO improperly and corrupts the SPI frame.
     */
    *(volatile uint8_t *)&SPI1->DR = data;

    /* Wait until the Receive Buffer Not Empty (RXNE) flag is set */
    while (!(SPI1->SR & SPI_SR_RXNE));

    /* Read the 8-bit response (clears RXNE automatically) */
    return *(volatile uint8_t *)&SPI1->DR;
}

/**
 * @brief   Dynamically changes the SPI baud rate prescaler.
 */
void SPI_SetSpeed(uint8_t fast)
{
    /* The SPI peripheral must be strictly disabled before modifying Baud Rate bits */
    SPI1->CR1 &= ~SPI_CR1_SPE;

    /* Clear the existing Baud Rate (BR) bitfield */
    SPI1->CR1 &= ~SPI_CR1_BR_Msk;

    if (fast) {
        /* High Speed: Set BR to fPCLK/16 (011).
           At 80MHz PCLK, this yields a 5 MHz SPI clock. */
        SPI1->CR1 |= (3U << SPI_CR1_BR_Pos);
    } else {
        /* Low Speed: Set BR to fPCLK/256 (111).
           At 80MHz PCLK, this yields a ~312.5 kHz SPI clock. */
        SPI1->CR1 |= (7U << SPI_CR1_BR_Pos);
    }

    /* Re-enable the SPI peripheral with the new speed */
    SPI1->CR1 |= SPI_CR1_SPE;
}
