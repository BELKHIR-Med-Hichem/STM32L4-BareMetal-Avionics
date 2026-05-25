/**
 * @file    SH1106.h
 * @author  Meh
 * @date    May 5, 2026
 * @brief   Bare-metal I2C driver for the SH1106 OLED display controller (128x64).
 * @details Provides functions to initialize the display, manage the cursor position
 * across pages and columns, and print text using a standard 5x8 ASCII font.
 */

#ifndef INC_SH1106_H_
#define INC_SH1106_H_

#include <stdint.h>
#include "I2C.h"

/**
 * @brief 8-bit I2C address of the SH1106 OLED.
 * @details Default 7-bit address is 0x3C, shifted left by 1 for STM32 HAL/bare-metal.
 */
#define SH1106_ADDR (0x3C << 1)

/**
 * @brief   Initializes the OLED display.
 * @details Sends the fundamental configuration sequence including charge pump enable,
 * multiplex ratio, display offset, and orientation mapping. Finally, clears
 * the screen and turns the display on.
 * @return  None
 */
void OLED_Init(void);

/**
 * @brief   Clears the entire screen.
 * @details Loops through all 8 pages (rows) and 128 columns, filling the Graphic
 * Display Data RAM (GDDRAM) with 0x00 (black).
 * @return  None
 */
void OLED_Clear(void);

/**
 * @brief   Sets the cursor position for the next text or drawing operation.
 * @param   page The row/page number (0 to 7). Each page is 8 pixels tall.
 * @param   col  The pixel column number (0 to 127).
 * @return  None
 */
void OLED_SetCursor(uint8_t page, uint8_t col);

/**
 * @brief   Prints a single ASCII character at the current cursor position.
 * @param   c The character to print (must be between ASCII 32 and 126).
 * @return  None
 */
void OLED_PrintChar(char c);

/**
 * @brief   Prints a null-terminated string starting at the current cursor position.
 * @param   str Pointer to the null-terminated character array.
 * @return  None
 */
void OLED_PrintString(char* str);

#endif /* INC_SH1106_H_ */
