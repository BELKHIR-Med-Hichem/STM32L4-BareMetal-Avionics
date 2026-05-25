/**
 * @file    SD.h
 * @author  Meh
 * @brief   Bare-metal SPI SD card driver with buffered raw sector logging.
 * @details This driver bypasses traditional file systems (like FAT32) to write
 * directly to the SD card's physical sectors via SPI.
 * * Logging strategy:
 * SD cards write in strict 512-byte blocks. Writing one small CSV line per block
 * wastes ~480 bytes every sample and severely wears out the flash. Instead, this
 * module keeps a 512-byte RAM buffer:
 * - SD_Log() appends text to RAM and auto-flushes to the SD card when the block is full.
 * - SD_Flush() pads the remaining space with 0x00 and forces a write (e.g., on power-off).
 * * Reading data back:
 * On Linux:   dd if=/dev/sdX bs=512 skip=100 count=<N> | strings
 * On Windows: HxD -> open physical disk -> jump to sector 100
 */

#ifndef SD_H_
#define SD_H_

#include "stm32l432xx.h"
#include "SPI.h"
#include <stdint.h>
#include <stdio.h>

/* ===============================================================
 * Core block-level API
 * =============================================================== */

/**
 * @brief   Initializes the SD card in SPI mode.
 * @details Performs the mandatory wake-up clocking, resets the card (CMD0),
 * verifies the voltage range (CMD8), and polls until the card is ready (ACMD41).
 * @return  uint8_t Status code:
 * - 0: Success
 * - 1: CMD0 failed (card did not enter SPI idle state)
 * - 2: ACMD41 timed out (card did not become ready)
 */
uint8_t SD_Init(void);

/**
 * @brief   Writes exactly 512 bytes to a raw block address on the SD card.
 * @param   blockAddress Sector number (logical block address).
 * @param   buffer Pointer to the 512-byte source buffer to be written.
 * @return  uint8_t Status code:
 * - 0: Success
 * - 1: CMD24 rejected by card
 * - 2: Data packet rejected (CRC or write error token received)
 * - 3: Write timed out (card held MISO low / busy for too long)
 */
uint8_t SD_WriteBlock(uint32_t blockAddress, const uint8_t *buffer);


/* ===============================================================
 * Buffered logging API
 * =============================================================== */

/**
 * @brief   Appends text/data to the internal 512-byte sector buffer.
 * @details When the buffer reaches 512 bytes, it is transparently written to
 * the SD card, and a new sector is started.
 * @param   data Pointer to the text/data to log (does not need to be NUL-terminated).
 * @param   len Number of bytes to append.
 * @return  uint8_t Returns 0 if buffered/written successfully, or the SD_WriteBlock error code.
 */
uint8_t SD_Log(const char *data, uint16_t len);

/**
 * @brief   Flushes (writes) any partial sector remaining in the RAM buffer.
 * @details Pads the remaining space in the 512-byte block with 0x00 and writes
 * it to the card. It is completely safe to call even if the buffer is empty.
 * @return  uint8_t Returns 0 on success (or if nothing to flush), or the SD_WriteBlock error code.
 */
uint8_t SD_Flush(void);

/**
 * @brief   Returns the next sector address that will be written.
 * @details Useful for diagnostic tracking or resuming logging after a microcontroller reset.
 * @return  uint32_t The current target sector index.
 */
uint32_t SD_GetCurrentSector(void);

#endif /* SD_H_ */
