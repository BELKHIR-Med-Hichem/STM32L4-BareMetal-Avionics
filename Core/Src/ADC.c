/**
 * @file    ADC.c
 * @author  Meh
 * @date    May 12, 2026
 * @brief   Implementation of ADC1 initialization and LM35 reading functions.
 * @details Contains the routines to properly spin up the ADC from power-down mode,
 * calibrate it, and fetch temperature conversions using an external LM35.
 */

#include "ADC.h"
#include "delay.h"
#include "USART.h"

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
void ADC1_Init(void)
{
    /* 0 - Configure GPIOB Pin 0 (PB0) for Analog mode */
    RCC->AHB2ENR |= RCC_AHB2ENR_GPIOBEN;
    GPIOB->MODER &= ~(3U << (0 * 2)); // Clear MODER bits for pin 0
    GPIOB->MODER |=  (3U << (0 * 2)); // Set to Analog mode (11)

    /* 1 - Enable the ADC clock on the AHB2 bus */
    RCC->AHB2ENR |= RCC_AHB2ENR_ADCEN;

    /* 2 - Set the ADC clock to HCLK/1 */
    ADC1_COMMON->CCR |= (1U << 16);

    /* 3 - Exit deep power-down mode and enable the ADC voltage regulator */
    ADC1->CR &= ~ADC_CR_DEEPPWD;
    ADC1->CR |= ADC_CR_ADVREGEN;
    delay_ms(20); // Wait for the regulator to stabilize

    /* 4 - Run calibration */
    ADC1->CR |= ADC_CR_ADCAL;
    while (ADC1->CR & ADC_CR_ADCAL); // Wait for the calibration to complete

    /* 5 - Enable ADC1 and wait for the ADRDY flag */
    ADC1->ISR |= ADC_ISR_ADRDY; // Clear the ready flag by writing 1 to it
    ADC1->CR  |= ADC_CR_ADEN;
    while (!(ADC1->ISR & ADC_ISR_ADRDY)); // Wait for the ADC to be ready

    /* 6 - Select channel 15 (PB0 for LM35) in SQR1 */
    ADC1->SQR1 &= ~ADC_SQR1_L;   // Clear regular sequence length (1 conversion)
    ADC1->SQR1 &= ~ADC_SQR1_SQ1; // Clear sequence 1 bits
    ADC1->SQR1 |= (15U << 6);    // Assign channel 15 to the 1st conversion in sequence

    /* 7 - Set the sampling time to 640.5 cycles (SMP = 7) for channel 15 */
    ADC1->SMPR2 &= ~(7U << 15);  // Clear SMP15 bits
    ADC1->SMPR2 |= (7U << 15);   // Set sampling time to 640.5 cycles
}

/**
 * @brief   Reads the analog value from the LM35 and computes the temperature.
 * @details Triggers a single ADC conversion, waits for the End of Conversion (EOC) flag,
 * and converts the 12-bit raw ADC value into a temperature in degrees Celsius.
 * The calculation assumes a 3.3V reference voltage and that the LM35
 * has a scale factor of 10mV/°C.
 * @return  float The calculated temperature in degrees Celsius.
 */
float ADC1_ReadLM35(void)
{
    int32_t rawValue;

    /* Start the conversion */
    ADC1->CR |= ADC_CR_ADSTART;

    /* Wait for the End of Conversion (EOC) flag to be set */
    while (!(ADC1->ISR & ADC_ISR_EOC));

    /* Read the converted value (reading DR automatically clears the EOC flag) */
    rawValue = ADC1->DR;

    /* Calculate temperature:
     * (rawValue * 3300.0f / 4095.0f) gives voltage in mV
     * Dividing by 10.0f accounts for the 10mV/°C LM35 scale factor
     */
    float temp = (rawValue * 3300.0f / 4095.0f) / 10.0f;

    return temp;
}
