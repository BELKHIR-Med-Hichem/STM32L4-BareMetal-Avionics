/**
 * @file    BMP280.h
 * @author  Meh
 * @date    Jan 25, 2026
 * @brief   Driver for the Bosch BMP280 barometric pressure and temperature sensor over I2C.
 * @details This module provides initialization and data acquisition for the BMP280.
 * It applies the official Bosch compensation formulas to convert raw ADC
 * values into calibrated physical measurements.
 * * The following configuration is applied during initialization:
 * - Temperature oversampling: x2
 * - Pressure oversampling:    x16
 * - IIR filter coefficient:   x4
 * - Standby time:             62.5ms
 * - Power Mode:               Normal
 * * @note    I2C1_Config() must be called before BMP280_Init().
 * BMP280_Temp() must always be called before BMP280_Pressure(), because
 * the pressure compensation formula depends on 't_fine', which is computed
 * inside the BMP280_Temp() function.
 */

#ifndef INC_BMP280_H_
#define INC_BMP280_H_

#include "stdint.h"
#include "I2C.h"

/**
 * @brief   Stores processed and calibrated measurements from the BMP280.
 */
typedef struct {
    float temperature;      /**< Calibrated temperature in degrees Celsius (°C) */
    float pressure;         /**< Calibrated pressure in hectopascals (hPa) */
    float Vertical_Speed;   /**< Calculated vertical speed/altitude (currently commented out in logic) */
} BMP280_Data_t;

/**
 * @brief   Initializes the BMP280 and loads the factory calibration coefficients.
 * @details The initialization executes the following sequence:
 * -# Reads the WHO_AM_I register (0xD0) and checks for the expected ID (0x58).
 * -# Reads the 24 factory calibration coefficients starting from register 0x88.
 * -# Writes to the CONFIG register: t_sb = 62.5ms, IIR filter = x4.
 * -# Writes to the CTRL register: osrs_t = x2, osrs_p = x16, mode = Normal.
 * @return  int8_t Returns 0 if successful, or -1 if the device ID does not match.
 */
int8_t BMP280_Init(void);

/**
 * @brief   Reads the raw temperature ADC output and converts it to °C.
 * @param   data Pointer to the BMP280_Data_t structure where the temperature will be stored.
 * @return  int8_t Returns 0 upon completion.
 */
int8_t BMP280_Temp(BMP280_Data_t *data);

/**
 * @brief   Reads the raw pressure ADC output and converts it to hPa.
 * @param   data Pointer to the BMP280_Data_t structure where the pressure will be stored.
 * @return  int8_t Returns 0 upon completion.
 */
int8_t BMP280_Pressure(BMP280_Data_t *data);

#endif /* INC_BMP280_H_ */
