/**
 * @file    SD.c
 * @author  Meh
 * @brief   Implementation of the bare-metal SPI SD card driver.
 */

#include "SD.h"
#include "delay.h"
#include <string.h>
#include <stdio.h>

/** @defgroup SD_CS_Control SPI Chip Select Macros
 * @{
 */
#define SD_CS_LOW()  (GPIOA->BSRR = (1U << (4 + 16))) /**< Pull PA4 Low (Select) */
#define SD_CS_HIGH() (GPIOA->BSRR = (1U << 4))        /**< Pull PA4 High (Deselect) */
/** @} */

/** @defgroup SD_Commands SD Card Standard SPI Commands
 * @{
 */
#define CMD0   0   /**< GO_IDLE_STATE: Resets the card and forces it into SPI mode */
#define CMD8   8   /**< SEND_IF_COND: Verifies operating voltage range */
#define CMD55  55  /**< APP_CMD: Prefix indicating the next command is an application-specific command */
#define ACMD41 41  /**< SD_SEND_OP_COND: Starts initialization and checks if the card is ready */
#define CMD24  24  /**< WRITE_BLOCK: Writes a single 512-byte block */
/** @} */

/* ================================================================== */
/* Logging state (File-scope variables)                              */
/* ================================================================== */
static uint8_t  logBuffer[512]; /**< RAM buffer accumulating the sector data */
static uint16_t logIndex  = 0;  /**< Current byte position in the buffer */
static uint32_t logSector = 100; /**< Starting physical sector for data logging */

/* ================================================================== */
/* Low-level hardware helpers                                        */
/* ================================================================== */

/**
 * @brief Initializes GPIOA Pin 4 as the SPI Chip Select (CS) for the SD card.
 */
static void SD_CS_Init(void)
{
    RCC->AHB2ENR |= RCC_AHB2ENR_GPIOAEN;

    GPIOA->MODER   &= ~(3U << (4 * 2));
    GPIOA->MODER   |=  (1U << (4 * 2));   /* Set PA4 to General Purpose Output */
    GPIOA->OSPEEDR |=  (3U << (4 * 2));   /* Set PA4 to High Speed */
    GPIOA->BSRR     =  (1U << 4);         /* Default HIGH (Deselected) */
}

/**
 * @brief   Sends a 6-byte command packet to the SD card over SPI.
 * @details An SD command packet consists of:
 * - 1 byte:  Command index OR'ed with 0x40 (start bit = 0, transmission bit = 1).
 * - 4 bytes: The command argument (MSB first).
 * - 1 byte:  CRC checksum and end bit (0x01).
 * @param   cmd The command index (e.g., CMD0).
 * @param   arg The 32-bit command argument.
 * @param   crc The pre-calculated CRC byte (only strictly required for CMD0 and CMD8).
 * @return  uint8_t The first non-0xFF R1 response byte received from the card.
 */
static uint8_t SD_SendCommand(uint8_t cmd, uint32_t arg, uint8_t crc)
{
    SPI_SendByte(0xFF); /* Guard byte before command */

    /* Transmit the 6-byte command structure */
    SPI_SendByte(cmd | 0x40);
    SPI_SendByte((arg >> 24) & 0xFF);
    SPI_SendByte((arg >> 16) & 0xFF);
    SPI_SendByte((arg >>  8) & 0xFF);
    SPI_SendByte( arg        & 0xFF);
    SPI_SendByte(crc);

    /* The SD card responds within 8 bytes. Poll until we get a valid response (not 0xFF). */
    uint8_t response;
    for (int i = 0; i < 8; i++) {
        response = SPI_TransferByte(0xFF);
        if (response != 0xFF) break;
    }
    return response;
}

/* ================================================================== */
/* Initialization                                                    */
/* ================================================================== */

/**
 * @brief   Initializes the SD card in SPI mode.
 */
uint8_t SD_Init(void)
{
    uint8_t  response;
    uint32_t timeout;

    SPI_Init();
    SD_CS_Init();

    /* ---- Step 1: Power-on ramp up ---- */
    /* Give the card at least 74 dummy clock cycles with CS High to stabilize */
    SD_CS_HIGH();
    for (int i = 0; i < 10; i++) SPI_SendByte(0xFF); /* 10 bytes = 80 clocks */

    /* ---- Step 2: CMD0 – Force into SPI idle state ---- */
    SD_CS_LOW();
    response = SD_SendCommand(CMD0, 0x00000000, 0x95); /* CRC 0x95 required for CMD0 */
    SD_CS_HIGH();
    SPI_SendByte(0xFF);
    printf("CMD0: 0x%02X\r\n", response);

    /* Expect 0x01 (In Idle State bit set) */
    if (response != 0x01) return 1;

    /* ---- Step 3: CMD8 – Verify voltage range (2.7-3.6V) ---- */
    SD_CS_LOW();
    response = SD_SendCommand(CMD8, 0x000001AA, 0x87); /* CRC 0x87 required for CMD8 */
    printf("CMD8: 0x%02X\r\n", response);

    if (response == 0x01) {
        /* Read the remaining 4 bytes of the R7 response. The last byte is the echo. */
        SPI_TransferByte(0xFF);
        SPI_TransferByte(0xFF);
        SPI_TransferByte(0xFF);
        uint8_t echo = SPI_TransferByte(0xFF);
        printf("CMD8 echo: 0x%02X (expect 0xAA)\r\n", echo);
    }
    SD_CS_HIGH();
    SPI_SendByte(0xFF);

    /* ---- Step 4: ACMD41 loop – Activate the card ---- */
    /* HCS (Host Capacity Support) bit 30 set to 1 to support SDHC/SDXC */
    timeout = 0;
    do {
        /* ACMD41 is preceded by CMD55 */
        SD_CS_LOW();
        SD_SendCommand(CMD55, 0x00000000, 0xFF);
        SD_CS_HIGH();
        SPI_SendByte(0xFF);

        SD_CS_LOW();
        response = SD_SendCommand(ACMD41, 0x40000000, 0xFF);
        SD_CS_HIGH();
        SPI_SendByte(0xFF);

        if (timeout % 100 == 0) {
            printf("ACMD41 [%lu]: 0x%02X\r\n", timeout, response);
        }

        timeout++;
        delay_ms(1);
    } while (response != 0x00 && timeout < 2000); /* Wait for Idle bit to clear (0x00) */

    printf("ACMD41 done: 0x%02X  timeout=%lu\r\n", response, timeout);

    if (response != 0x00) return 2;

    /* (Optional: Here you could reconfigure your SPI peripheral to a higher speed) */

    SPI_SendByte(0xFF);
    return 0;
}

/* ================================================================== */
/* Raw Sector Writing                                                */
/* ================================================================== */

/**
 * @brief   Writes exactly 512 bytes to the given block address.
 */
uint8_t SD_WriteBlock(uint32_t blockAddress, const uint8_t *buffer)
{
    uint8_t  response;
    uint32_t timeout = 0;

    SD_CS_LOW();

    /* Send the Write Block command (CMD24) */
    response = SD_SendCommand(CMD24, blockAddress, 0xFF);
    if (response != 0x00) {
        SD_CS_HIGH();
        printf("CMD24 rejected: 0x%02X\r\n", response);
        return 1;
    }

    /* Give the card a few cycles to prepare its internal buffer */
    SPI_SendByte(0xFF);
    SPI_SendByte(0xFF);
    SPI_SendByte(0xFF);

    /* Transmit the Data Start Token */
    SPI_SendByte(0xFE);

    /* Transmit exactly 512 bytes of payload */
    for (uint32_t i = 0; i < 512; i++) {
        SPI_SendByte(buffer[i]);
    }

    /* Transmit dummy CRC (2 bytes) */
    SPI_SendByte(0xFF);
    SPI_SendByte(0xFF);

    /* Read the Data Response Token (Format: xxx0R101) */
    for (int i = 0; i < 8; i++) {
        response = SPI_TransferByte(0xFF);
        if (response != 0xFF) break;
    }

    /* 0x05 indicates the data was accepted */
    if ((response & 0x1F) != 0x05) {
        SD_CS_HIGH();
        printf("Data rejected: 0x%02X\r\n", response);
        return 2;
    }

    /* Wait for the busy signal to clear (SD card holds MISO low while programming flash) */
    while (SPI_TransferByte(0xFF) == 0x00) {
        if (++timeout > 0xFFFFF) {
            SD_CS_HIGH();
            return 3;
        }
    }

    SD_CS_HIGH();
    SPI_SendByte(0xFF);   /* 8 extra clocks to release bus */
    return 0;
}

/* ================================================================== */
/* Buffered Logging Handlers                                         */
/* ================================================================== */

/**
 * @brief   Appends text to the internal buffer and writes to SD when full.
 */
uint8_t SD_Log(const char *data, uint16_t len)
{
    uint16_t remaining = len;
    const char *src    = data;

    while (remaining > 0) {
        /* Calculate how much space is left in the current 512-byte chunk */
        uint16_t space = 512 - logIndex;
        uint16_t chunk = (remaining < space) ? remaining : space;

        /* Copy data into the RAM buffer */
        memcpy(logBuffer + logIndex, src, chunk);
        logIndex  += chunk;
        src       += chunk;
        remaining -= chunk;

        /* If the buffer is full, dump it to the physical SD sector */
        if (logIndex == 512) {
            uint8_t r = SD_WriteBlock(logSector, logBuffer);
            if (r != 0) {
                printf("SD_Log write fail: sector=%lu err=%u\r\n", logSector, r);
                return r;
            }

            /* Increment physical sector address and reset buffer */
            logSector++;
            logIndex = 0;
            memset(logBuffer, 0x00, 512);
        }
    }
    return 0;
}

/**
 * @brief   Flushes remaining buffer contents to the SD card.
 */
uint8_t SD_Flush(void)
{
    /* If the buffer is entirely empty, do nothing */
    if (logIndex == 0) return 0;

    /* Pad the rest of the block with null bytes (0x00) to complete the 512-byte requirement */
    memset(logBuffer + logIndex, 0x00, 512 - logIndex);

    /* Write the padded block to the card */
    uint8_t r = SD_WriteBlock(logSector, logBuffer);
    if (r != 0) {
        printf("SD_Flush write fail: sector=%lu err=%u\r\n", logSector, r);
        return r;
    }

    /* Prepare for the next potential write session */
    logSector++;
    logIndex = 0;
    memset(logBuffer, 0x00, 512);

    return 0;
}

/**
 * @brief   Returns the next physical sector that will be written.
 */
uint32_t SD_GetCurrentSector(void)
{
    return logSector;
}
