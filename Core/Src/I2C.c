/**
 * @file    I2C.c
 * @author  Meh
 * @date    Jan 20, 2026
 * @brief   Implementation of the I2C1 hardware driver.
 */

#include "I2C.h"

/**
 * @brief   Initializes the I2C1 peripheral at 400kHz (Fast Mode).
 */
void I2C1_Config(void)
{
    /* 1 - Enable I2C and set the GPIO clock */
    RCC->APB1ENR1 |= RCC_APB1ENR1_I2C1EN;
    RCC->AHB2ENR  |= RCC_AHB2ENR_GPIOBEN;

    /* 2 - Set the I2C ports (PB6 and PB7) to Alternate Mode (10) */
    GPIOB->MODER &= ~(3U << (6 * 2)); // Reset PB6
    GPIOB->MODER |=  (2U << (6 * 2));
    GPIOB->MODER &= ~(3U << (7 * 2)); // Reset PB7
    GPIOB->MODER |=  (2U << (7 * 2));

    /* 3 - Set the I2C port's output types to Open Drain */
    GPIOB->OTYPER |= (1U << 6); // PB6
    GPIOB->OTYPER |= (1U << 7); // PB7

    /* 4 - Set the I2C ports to High Speed */
    GPIOB->OSPEEDR |= (3U << (6 * 2));
    GPIOB->OSPEEDR |= (3U << (7 * 2));

    /* 5 - Set the internal Pull-up resistors */
    GPIOB->PUPDR &= ~(3U << (6 * 2));
    GPIOB->PUPDR |=  (1U << (6 * 2));
    GPIOB->PUPDR &= ~(3U << (7 * 2));
    GPIOB->PUPDR |=  (1U << (7 * 2));

    /* 6 - Assign Alternate Function 4 (AF4) for I2C1 to PB6 and PB7 */
    GPIOB->AFR[0] &= ~(0xF << (6 * 4)); // PB6
    GPIOB->AFR[0] |=  (4U << (6 * 4));
    GPIOB->AFR[0] &= ~(0xF << (7 * 4)); // PB7
    GPIOB->AFR[0] |=  (4U << (7 * 4));

    /* 7 - Software reset the I2C by disabling PE (Peripheral Enable) */
    I2C1->CR1 &= ~I2C_CR1_PE;

    /* 8 - Set the I2C1 to Fast Mode (400 kHz @ 80MHz PCLK) */
    I2C1->TIMINGR = 0x00F12981;

    /* Re-enable the peripheral */
    I2C1->CR1 |= I2C_CR1_PE;
}

/**
 * @brief   Writes a single byte of data to an I2C device.
 */
void I2C1_Write(uint8_t slaveAddr, uint8_t data)
{
    /* Configure CR2 for 1 byte transmission, auto-end mode, and generate START */
    I2C1->CR2 = (slaveAddr & I2C_CR2_SADD) | (1 << 16) | I2C_CR2_AUTOEND | I2C_CR2_START;

    /* Wait for TXIS (Transmit Interrupt Status) flag to indicate TXDR is empty */
    while (!(I2C1->ISR & I2C_ISR_TXIS));

    /* Send data byte */
    I2C1->TXDR = data;

    /* Wait for STOP condition flag */
    while (!(I2C1->ISR & I2C_ISR_STOPF));

    /* Clear STOP flag and clean up CR2 */
    I2C1->ICR |= I2C_ICR_STOPCF;
    I2C1->CR2 = 0;
}

/**
 * @brief   Writes a single byte of data to a specific register on an I2C device.
 */
void I2C1_WriteReg(uint8_t slaveAddr, uint8_t regAddr, uint8_t data)
{
    /* Configure CR2 for 2 bytes transmission (Address + Data), auto-end, and generate START */
    I2C1->CR2 = (slaveAddr & I2C_CR2_SADD) | (2 << 16) | I2C_CR2_AUTOEND | I2C_CR2_START;

    /* Wait for TXIS flag to indicate TXDR is empty */
    while (!(I2C1->ISR & I2C_ISR_TXIS));

    /* Send the register address */
    I2C1->TXDR = regAddr;

    /* Wait for TXIS flag again */
    while (!(I2C1->ISR & I2C_ISR_TXIS));

    /* Send the data byte */
    I2C1->TXDR = data;

    /* Wait for STOP condition flag */
    while (!(I2C1->ISR & I2C_ISR_STOPF));

    /* Clear STOP flag and clean up CR2 */
    I2C1->ICR |= I2C_ICR_STOPCF;
    I2C1->CR2 = 0;
}

/**
 * @brief   Reads one or multiple bytes from a specific register on an I2C device.
 */
void I2C1_Read(uint8_t slaveAddr, uint8_t regAddr, uint8_t *data, uint8_t length)
{
    /* --- WRITE PHASE (Send Register Address) --- */
    /* Generate START with 1 byte to send (no auto-end to allow Repeated Start) */
    I2C1->CR2 = (slaveAddr & I2C_CR2_SADD) | (1 << 16) | I2C_CR2_START;

    /* Wait for TXIS flag */
    while (!(I2C1->ISR & I2C_ISR_TXIS));

    /* Send the register address */
    I2C1->TXDR = regAddr;

    /* Wait for Transfer Complete (TC) flag instead of TXIS because AUTOEND is 0 */
    while (!(I2C1->ISR & I2C_ISR_TC));

    /* --- READ PHASE (Fetch Data) --- */
    /* Generate Repeated Start with RD_WRN=1 (Read) and auto-end enabled */
    I2C1->CR2 = (slaveAddr & I2C_CR2_SADD) | (length << 16) | I2C_CR2_RD_WRN | I2C_CR2_START | I2C_CR2_AUTOEND;

    for (uint8_t i = 0; i < length; i++)
    {
        /* Wait until RXNE (Receive Data Register Not Empty) is set */
        while (!(I2C1->ISR & I2C_ISR_RXNE));

        /* Store the received byte */
        data[i] = (uint8_t)I2C1->RXDR;
    }

    /* Wait for STOP condition flag */
    while (!(I2C1->ISR & I2C_ISR_STOPF));

    /* Clear STOP flag and clean up CR2 */
    I2C1->ICR |= I2C_ICR_STOPCF;
    I2C1->CR2 = 0;
}
