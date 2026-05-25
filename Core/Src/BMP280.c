/**
 * @file    BMP280.c
 * @author  Meh
 * @date    Jan 25, 2026
 * @brief   Implementation of the BMP280 initialization and data reading functions.
 */

#include "BMP280.h"
#include "delay.h"
#include "stdio.h"
#include <math.h>

/** * @defgroup BMP280_Registers BMP280 Register Map
 * @brief    Internal register addresses and expected constants used by this driver.
 * @{
 */
#define BMP280_ADDR          (0x76 << 1)  /**< 8-bit I2C address of the BMP280 (SDO connected to GND) */
#define BMP280_CHIP_ID       0x58         /**< Expected return value from the WHO_AM_I register */
#define BMP280_REG_ID        0xD0         /**< WHO_AM_I register address */
#define BMP280_REG_CALIB     0x88         /**< Start address of the calibration data */
#define BMP280_REG_CTRL      0xF4         /**< Control register (bits: 7-5 osrs_t, 4-2 osrs_p, 1-0 mode) */
#define BMP280_REG_CONFIG    0xF5         /**< Configuration register (bits: 7-5 t_sb, 4-2 filter, 0 spi3w_en) */
#define BMP280_REG_PRESS_MSB 0xF7         /**< Start address of pressure ADC output */
#define BMP280_REG_TEMP_MSB  0xFA         /**< Start address of temperature ADC output */
/** @} */

/**
 * @brief Factory calibration coefficients read from the sensor's NVM.
 */
static struct {
    uint16_t dig_T1;
    int16_t  dig_T2;
    int16_t  dig_T3;
    uint16_t dig_P1;
    int16_t  dig_P2;
    int16_t  dig_P3;
    int16_t  dig_P4;
    int16_t  dig_P5;
    int16_t  dig_P6;
    int16_t  dig_P7;
    int16_t  dig_P8;
    int16_t  dig_P9;
    int32_t  t_fine;  /**< Intermediate temperature value shared with the pressure formula */
} calib;

/**
 * @brief   Initializes the BMP280 and loads the factory calibration coefficients.
 */
int8_t BMP280_Init(void)
{
    uint8_t id = 0;
    printf("BMP280: Initializing...\r\n");

    /* 1 - Read the WHO_AM_I register and check for 0x58 */
    I2C1_Read(BMP280_ADDR, BMP280_REG_ID, &id, 1);

    if (id != BMP280_CHIP_ID)
    {
        printf("BMP280 not found (ID: 0x%X). Retrying...\r\n", id);
        return -1;
    }

    printf("BMP280: detected...Configuring...\r\n");

    /* 2 - Read the 24 factory calibration coefficients starting from 0x88 */
    uint8_t trim[24];
    I2C1_Read(BMP280_ADDR, BMP280_REG_CALIB, trim, 24);

    // Parse Temperature coefficients
    calib.dig_T1 = (uint16_t)(trim[1] << 8 | trim[0]);
    calib.dig_T2 = (int16_t)(trim[3] << 8 | trim[2]);
    calib.dig_T3 = (int16_t)(trim[5] << 8 | trim[4]);

    // Parse Pressure coefficients
    calib.dig_P1 = (uint16_t)(trim[7] << 8 | trim[6]);
    calib.dig_P2 = (int16_t)(trim[9] << 8 | trim[8]);
    calib.dig_P3 = (int16_t)(trim[11] << 8 | trim[10]);
    calib.dig_P4 = (int16_t)(trim[13] << 8 | trim[12]);
    calib.dig_P5 = (int16_t)(trim[15] << 8 | trim[14]);
    calib.dig_P6 = (int16_t)(trim[17] << 8 | trim[16]);
    calib.dig_P7 = (int16_t)(trim[19] << 8 | trim[18]);
    calib.dig_P8 = (int16_t)(trim[21] << 8 | trim[20]);
    calib.dig_P9 = (int16_t)(trim[23] << 8 | trim[22]);

    /* 3 - Write the CONFIG register (t_sb = 62.5ms, IIR filter = x4 -> 00101000 or 0x28) */
    I2C1_WriteReg(BMP280_ADDR, BMP280_REG_CONFIG, 0x28);

    /* 4 - Write the CTRL register (osrs_t = x2, osrs_p = x16, mode = Normal -> 01010111 or 0x57) */
    I2C1_WriteReg(BMP280_ADDR, BMP280_REG_CTRL, 0x57);

    delay_ms(100);
    return 0;
}

/**
 * @brief   Reads the raw temperature ADC output and converts it to °C.
 */
int8_t BMP280_Temp(BMP280_Data_t *data)
{
    uint8_t raw[3];

    /* Read the 3 temperature ADC bytes (0xFA = MSB, 0xFB = LSB, 0xFC = XLSB) */
    I2C1_Read(BMP280_ADDR, BMP280_REG_TEMP_MSB, raw, 3);

    int32_t adc_T = (int32_t)((raw[0] << 12) | (raw[1] << 4) | (raw[2] >> 4));

    /* Official Bosch 32-bit compensation formula */
    int32_t var1, var2;
    var1 = ((((adc_T >> 3) - ((int32_t)calib.dig_T1 << 1))) * ((int32_t)calib.dig_T2)) >> 11;
    var2 = (((((adc_T >> 4) - ((int32_t)calib.dig_T1)) * ((adc_T >> 4) - ((int32_t)calib.dig_T1))) >> 12) * ((int32_t)calib.dig_T3)) >> 14;

    calib.t_fine = var1 + var2;

    float T = (calib.t_fine * 5 + 128) >> 8;
    data->temperature = T / 100.0f;

    return 0;
}

/**
 * @brief   Reads the raw pressure ADC output and converts it to hPa.
 */
int8_t BMP280_Pressure(BMP280_Data_t *data)
{
    uint8_t raw[3];

    /* Read the 3 pressure ADC bytes (0xF7 = MSB, 0xF8 = LSB, 0xF9 = XLSB) */
    I2C1_Read(BMP280_ADDR, BMP280_REG_PRESS_MSB, raw, 3);

    int32_t adc_P = (int32_t)((raw[0] << 12) | (raw[1] << 4) | (raw[2] >> 4));

    /* Official Bosch 64-bit compensation formula */
    int64_t var1, var2, p;
    var1 = ((int64_t)calib.t_fine) - 128000;
    var2 = var1 * var1 * (int64_t)calib.dig_P6;
    var2 = var2 + ((var1 * (int64_t)calib.dig_P5) << 17);
    var2 = var2 + (((int64_t)calib.dig_P4) << 35);
    var1 = ((var1 * var1 * (int64_t)calib.dig_P3) >> 8) + ((var1 * (int64_t)calib.dig_P2) << 12);
    var1 = (((((int64_t)1) << 47) + var1)) * ((int64_t)calib.dig_P1) >> 33;

    /* Guard against division by zero */
    if (var1 == 0) {
        return 0;
    }

    p = 1048576 - adc_P;
    p = (((p << 31) - var2) * 3125) / var1;
    var1 = (((int64_t)calib.dig_P9) * (p >> 13) * (p >> 13)) >> 25;
    var2 = (((int64_t)calib.dig_P8) * p) >> 19;
    p = ((p + var1 + var2) >> 8) + (((int64_t)calib.dig_P7) << 4);
    p = p / 25600.0f;

    data->pressure = (float)p;

    /* Altitude conversion (Currently commented out in original logic)
    float altitude =  44330.0f * (1.0f - powf(p / 1013.25f, 0.190295f));
    altitude = altitude * 3.28084f;
    data->Vertical_Speed = altitude;
    */

    return 0;
}
