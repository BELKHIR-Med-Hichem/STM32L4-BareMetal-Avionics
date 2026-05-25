/**
 * @file    SPI.h
 * @author  Meh
 * @date    Mar 2, 2026
 * @brief   Bare-metal SPI1 driver for the STM32L432xx.
 * @details Provides fundamental block-polling functions to initialize, read,
 * write, and transfer data over SPI1. Includes a dynamic speed-switching
 * function essential for devices like SD cards that require a slow initialization
 * clock before switching to high-speed transfers.
 */

#ifndef INC_SPI_H_
#define INC_SPI_H_

#include <stdint.h>

/**
 * @brief   Initializes the SPI1 peripheral in Master mode.
 * @details Configures PB3 (SCK), PB4 (MISO), and PB5 (MOSI).
 * Sets the initial clock speed to extremely slow (fPCLK/256), configures
 * 8-bit data frames, and uses Software Slave Management (SSM).
 * @return  None
 */
void SPI_Init(void);

/**
 * @brief   Transmits a single byte over SPI.
 * @details Wraps the transfer function but discards the received byte.
 * @param   data The 8-bit payload to transmit.
 * @return  None
 */
void SPI_SendByte(uint8_t data);

/**
 * @brief   Reads a single byte over SPI.
 * @details Wraps the transfer function by sending a dummy byte to clock in data.
 * @param   data The dummy byte to send (typically 0xFF).
 * @return  uint8_t The received 8-bit payload.
 */
uint8_t SPI_ReadByte(uint8_t data);

/**
 * @brief   Transmits and receives a byte simultaneously (Full-Duplex).
 * @details Blocks until the Transmit buffer is empty (TXE), writes the byte,
 * then blocks until the Receive buffer is not empty (RXNE), and returns the data.
 * @param   data The 8-bit payload to transmit.
 * @return  uint8_t The received 8-bit payload.
 */
uint8_t SPI_TransferByte(uint8_t data);

/**
 * @brief   Dynamically changes the SPI baud rate prescaler.
 * @details Safely disables the SPI peripheral, alters the baud rate,
 * and re-enables it.
 * @param   fast If 1, sets speed to High (fPCLK/16). If 0, sets to Low (fPCLK/256).
 * @return  None
 */
void SPI_SetSpeed(uint8_t fast);

#endif /* INC_SPI_H_ */
