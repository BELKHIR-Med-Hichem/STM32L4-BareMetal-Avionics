/**
 * @file    I2C.h
 * @author  Meh
 * @date    Jan 20, 2026
 * @brief   I2C1 bare-metal driver for the STM32L412KB.
 * @details This module configures and drives the I2C1 peripheral on PB6 (SCL)
 * and PB7 (SDA) in Fast Mode at 400kHz with an 80MHz PCLK.
 * It provides basic blocking functions to read and write bytes
 * and registers.
 * @note    I2C1_Config() must be called before any I2C transaction.
 */

#ifndef INC_I2C_H_
#define INC_I2C_H_

#include "stdint.h"
#include "Rcc_Config.h"

/**
 * @brief   Initializes the I2C1 peripheral at 400kHz (Fast Mode).
 * @details Enables GPIOB and I2C1 clocks, configures PB6 (SCL) and PB7 (SDA)
 * as open-drain alternate functions (AF4), and applies the timing
 * register value for 400kHz at 80MHz PCLK.
 * @note    Must be called once before any I2C transaction.
 * Sys_CLK_Config() must have been called first to set the 80MHz PCLK.
 * @return  None
 */
void I2C1_Config(void);

/**
 * @brief   Writes a single byte of data to an I2C device.
 * @param   slaveAddr   7-bit I2C slave address.
 * @param   data        Byte to transmit.
 * @return  None
 */
void I2C1_Write(uint8_t slaveAddr, uint8_t data);

/**
 * @brief   Writes a single byte of data to a specific register on an I2C device.
 * @param   slaveAddr   7-bit I2C slave address.
 * @param   regAddr     Register address to write to.
 * @param   data        Byte to write into the register.
 * @return  None
 */
void I2C1_WriteReg(uint8_t slaveAddr, uint8_t regAddr, uint8_t data);

/**
 * @brief   Reads one or multiple bytes from a specific register on an I2C device.
 * @details Uses a Repeated Start condition between the write phase (register address)
 * and the read phase (fetching data).
 * @param   slaveAddr   7-bit I2C slave address.
 * @param   regAddr     Register address to read from.
 * @param   data        Pointer to the buffer where received bytes will be stored.
 * @param   length      Number of bytes to read.
 * @return  None
 */
void I2C1_Read(uint8_t slaveAddr, uint8_t regAddr, uint8_t *data, uint8_t length);

#endif /* INC_I2C_H_ */
