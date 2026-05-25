/**
 * @file    ADC.h
 * @author  Meh
 * @date    May 12, 2026
 * @brief   Header file for Analog-to-Digital Converter (ADC) configuration and control.
 * @details This file provides the function prototypes for initializing ADC1
 * and reading temperature data from an external LM35 temperature sensor.
 */

#ifndef INC_ADC_H_
#define INC_ADC_H_

#include "stm32l432xx.h"

/**
 * @brief   Configures ADC1 to sample an external LM35 temperature sensor on PB0.
 * @details The initialization executes the following sequence:
 * -# Enables the GPIOB clock and sets PB0 to analog mode.
 * -# Enables the ADC clock on the AHB2 bus.
 * -# Sets the ADC clock to HCLK/1.
 * -# Exits deep power-down mode and enables the ADC voltage regulator.
 * -# Runs ADC calibration for accurate readings.
 * -# Enables ADC1 and waits for the ADRDY (ADC Ready) flag.
 * -# Selects Channel 15 (PB0) as the first conversion in the sequence (SQR1).
 * -# Sets the sampling time for Channel 15 to 640.5 cycles (SMP = 7).
 * @return  None
 */
void ADC1_Init(void);

/**
 * @brief   Reads the analog value from the LM35 and computes the temperature.
 * @details Triggers a single ADC conversion, waits for the End of Conversion (EOC) flag,
 * and converts the 12-bit raw ADC value into a temperature in degrees Celsius.
 * The calculation assumes a 3.3V reference voltage and that the LM35
 * has a scale factor of 10mV/°C.
 * @return  float The calculated temperature in degrees Celsius.
 */
float ADC1_ReadLM35(void);

#endif /* INC_ADC_H_ */
