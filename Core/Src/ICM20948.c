/**
 * @file    ICM20948.c
 * @author  Meh
 * @date    May 5, 2026
 * @brief   Implementation of the ICM-20948 driver functions.
 */

#include "ICM20948.h"
#include <stdio.h>
#include <math.h>

/** @defgroup ICM20948_Registers ICM-20948 Register Map
 * @brief    Internal register addresses and constants.
 * @{
 */
#define ICM20948_ADDR       (0x69 << 1) /**< 8-bit I2C address (AD0 = 1) */
#define REG_BANK_SEL        0x7F        /**< Register to change user banks */

/* ---------- BANK 0 REGISTERS ---------- */
#define WHO_AM_I_REG        0x00
#define PWR_MGMT_1_REG      0x06
#define ACCEL_XOUT_H_REG    0x2D
#define GYRO_XOUT_H_REG     0x33
#define TEMP_OUT_H_REG      0x39

/* ---------- BANK 2 REGISTERS ---------- */
#define GYRO_SMPLRT_DIV     0x00
#define GYRO_CONFIG_1       0x01
#define ACCEL_SMPLRT_DIV_1  0x10
#define ACCEL_CONFIG        0x14
/** @} */

/** @defgroup ICM20948_Sensitivities Conversion Factors
 * @brief    Scale factors based on the selected full-scale ranges.
 * @{
 */
#define SENSITIVITY_2G      16384.0f    /**< LSB/g for ±2g range */
#define SENSITIVITY_GYRO    131.0f      /**< LSB/(°/s) for ±250dps range */
#define SENSITIVITY_TEMP    333.87f     /**< ICM-20948 specific temperature scale */
#define TEMP_OFFSET         21.0f       /**< ICM-20948 specific temperature offset */
#define RAD_TO_DEG          57.295779513f /**< Multiplier to convert radians to degrees */
/** @} */

/* Global variables for gyroscope zero-rate offsets */
int16_t off_gyro_X = 0, off_gyro_Y = 0, off_gyro_Z = 0;

/**
 * @brief   Helper function to switch ICM-20948 register banks.
 * @details The ICM-20948 organizes its registers into 4 user banks.
 * This function writes to the REG_BANK_SEL register to switch between them.
 * @param   bank The target bank number (0 to 3).
 */
void ICM20948_Select_Bank(uint8_t bank)
{
    I2C1_WriteReg(ICM20948_ADDR, REG_BANK_SEL, (bank << 4));
}

/**
 * @brief   Initializes the ICM-20948 sensor.
 */
int8_t ICM20948_Init(void)
{
    uint8_t whoAmI = 0;
    printf("System initialization...\r\n");

    /* Ensure we are in Bank 0 to read WHO_AM_I */
    ICM20948_Select_Bank(0);
    I2C1_Read(ICM20948_ADDR, WHO_AM_I_REG, &whoAmI, 1);

    /* ICM-20948 WHO_AM_I default value is 0xEA */
    if (whoAmI != 0xEA) {
        printf("ICM20948: Not found! (ID: 0x%02X)\r\n", whoAmI);
        return -1;
    }
    printf("ICM20948 Detected. Reseting...\r\n");

    /* Device Reset: Set H_RESET bit in PWR_MGMT_1 */
    I2C1_WriteReg(ICM20948_ADDR, PWR_MGMT_1_REG, 0x80);
    delay_ms(100);

    /* Power on and set clock to auto-select (recommended for best accuracy) */
    I2C1_WriteReg(ICM20948_ADDR, PWR_MGMT_1_REG, 0x01);
    delay_ms(50);

    /* Switch to Bank 2 to configure sensors */
    ICM20948_Select_Bank(2);

    /* Config Gyro: ±250 dps, enable Digital Low Pass Filter (DLPF) */
    I2C1_WriteReg(ICM20948_ADDR, GYRO_CONFIG_1, 0x00);

    /* Config Accel: ±2g, enable Digital Low Pass Filter (DLPF) */
    I2C1_WriteReg(ICM20948_ADDR, ACCEL_CONFIG, 0x00);

    /* Config Sample Rates (Divider) */
    I2C1_WriteReg(ICM20948_ADDR, GYRO_SMPLRT_DIV, 0x09);

    /* Switch back to Bank 0 for normal reading operations */
    ICM20948_Select_Bank(0);

    /* Automatically trigger gyroscope calibration */
    ICM20948_gyro_Calibrate();

    return 0;
}

/**
 * @brief   Calibrates the gyroscope by calculating the static offsets.
 */
int8_t ICM20948_gyro_Calibrate(void)
{
    printf("Calibration of the gyroscope on going... DO NOT MOVE! \r\n");

    long sumX = 0, sumY = 0, sumZ = 0;
    uint8_t rawData[6];
    int samples = 200;

    /* Accumulate 200 samples with a 5ms delay between each (~1 second total) */
    for(int i = 0; i < samples; i++)
    {
        I2C1_Read(ICM20948_ADDR, GYRO_XOUT_H_REG, rawData, 6);
        sumX += (int16_t)(rawData[0] << 8 | rawData[1]);
        sumY += (int16_t)(rawData[2] << 8 | rawData[3]);
        sumZ += (int16_t)(rawData[4] << 8 | rawData[5]);
        delay_ms(5);
    }

    /* Calculate the average offset for each axis */
    off_gyro_X = sumX / samples;
    off_gyro_Y = sumY / samples;
    off_gyro_Z = sumZ / samples;

    printf("Calibration of the gyroscope completed.\r\n");
    return 0;
}

/**
 * @brief   Reads the raw accelerometer data and scales it to physical values (g).
 */
void ICM20948_accel(ICM_Data_t *data)
{
    uint8_t buffer[6];

    /* Read 6 bytes starting from ACCEL_XOUT_H */
    I2C1_Read(ICM20948_ADDR, ACCEL_XOUT_H_REG, buffer, 6);

    /* Combine High and Low bytes */
    int16_t raw_X = (int16_t)(buffer[0] << 8 | buffer[1]);
    int16_t raw_Y = (int16_t)(buffer[2] << 8 | buffer[3]);
    int16_t raw_Z = (int16_t)(buffer[4] << 8 | buffer[5]);

    /* Scale to physical values (g) */
    data->accel_X = (float)raw_X / SENSITIVITY_2G;
    data->accel_Y = (float)raw_Y / SENSITIVITY_2G;
    data->accel_Z = (float)raw_Z / SENSITIVITY_2G;
}

/**
 * @brief   Reads the raw gyroscope data, applies offsets, and scales to °/s.
 */
void ICM20948_gyro(ICM_Data_t *data)
{
    uint8_t buffer[6];

    /* Read 6 bytes starting from GYRO_XOUT_H */
    I2C1_Read(ICM20948_ADDR, GYRO_XOUT_H_REG, buffer, 6);

    /* Combine High and Low bytes */
    int16_t raw_X = (int16_t)(buffer[0] << 8 | buffer[1]);
    int16_t raw_Y = (int16_t)(buffer[2] << 8 | buffer[3]);
    int16_t raw_Z = (int16_t)(buffer[4] << 8 | buffer[5]);

    /* Subtract calibration offsets and scale to degrees per second */
    data->gyro_X = (float)(raw_X - off_gyro_X) / SENSITIVITY_GYRO;
    data->gyro_Y = (float)(raw_Y - off_gyro_Y) / SENSITIVITY_GYRO;
    data->gyro_Z = (float)(raw_Z - off_gyro_Z) / SENSITIVITY_GYRO;
}

/**
 * @brief   Reads the raw temperature data and converts it to °C.
 */
void ICM20948_temp(ICM_Data_t *data)
{
    uint8_t buffer[2];

    /* Read 2 bytes starting from TEMP_OUT_H */
    I2C1_Read(ICM20948_ADDR, TEMP_OUT_H_REG, buffer, 2);

    /* Combine High and Low bytes */
    int16_t raw_temp = (int16_t)(buffer[0] << 8 | buffer[1]);

    /* Apply ICM-20948 Specific Formula to get °C */
    data->temp = ((float)raw_temp / SENSITIVITY_TEMP) + TEMP_OFFSET;
}

/**
 * @brief   Computes roll and pitch angles using a complementary filter.
 */
void ICM20948_Compute_Angles(ICM_Data_t *raw_data, ICM_Angles_t *angles, float dt)
{
    /* Calculate absolute orientation using accelerometer data (trigonometry) */
    float accel_roll = atan2(raw_data->accel_Y, raw_data->accel_Z) * RAD_TO_DEG;
    float accel_pitch = atan2(-raw_data->accel_X, sqrt(raw_data->accel_Y * raw_data->accel_Y + raw_data->accel_Z * raw_data->accel_Z)) * RAD_TO_DEG;

    /* Complementary Filter constant (Alpha).
       Determines the weighting between gyroscope and accelerometer data. */
    float alpha = 0.96f;

    /* * Filter equations:
     * High-pass filter on Gyroscope (Trust gyro for short term changes)
     * Low-pass filter on Accelerometer (Trust accel for long term absolute reference)
     */
    angles->roll = alpha * (angles->roll + raw_data->gyro_X * dt) + (1.0f - alpha) * accel_roll;
    angles->pitch = alpha * (angles->pitch + raw_data->gyro_Y * dt) + (1.0f - alpha) * accel_pitch;
}
