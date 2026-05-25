/**
 * @file    ICM20948.h
 * @author  Meh
 * @date    May 5, 2026
 * @brief   Driver for the InvenSense ICM-20948 9-axis motion tracking device.
 * @details This module provides functions to initialize the sensor, calibrate
 * the gyroscope, and read raw/scaled data for acceleration, angular rate,
 * and temperature. It also includes a complementary filter implementation
 * for calculating orientation angles.
 */

#ifndef INC_ICM20948_H_
#define INC_ICM20948_H_

#include "I2C.h"
#include "Rcc_Config.h"
#include "delay.h"
#include "USART.h"
#include <math.h>
#include <stdlib.h>

/**
 * @brief Structure to store physical measurements from the ICM-20948.
 */
typedef struct {
    float accel_X; /**< Acceleration along the X axis in g */
    float accel_Y; /**< Acceleration along the Y axis in g */
    float accel_Z; /**< Acceleration along the Z axis in g */
    float gyro_X;  /**< Angular rate around the X axis in degrees per second (°/s) */
    float gyro_Y;  /**< Angular rate around the Y axis in degrees per second (°/s) */
    float gyro_Z;  /**< Angular rate around the Z axis in degrees per second (°/s) */
    float temp;    /**< Die temperature in degrees Celsius (°C) */
} ICM_Data_t;

/**
 * @brief Structure to store calculated orientation angles.
 */
typedef struct {
    float roll;  /**< Rotation around the X axis in degrees */
    float pitch; /**< Rotation around the Y axis in degrees */
} ICM_Angles_t;

/**
 * @brief   Initializes the ICM-20948 sensor.
 * @details Checks the WHO_AM_I register, performs a software reset, wakes up
 * the device, sets up accelerometer/gyroscope configurations (±2g, ±250dps),
 * and triggers a gyroscope calibration.
 * @return  int8_t Returns 0 on success, -1 if the device ID is incorrect.
 */
int8_t ICM20948_Init(void);

/**
 * @brief   Calibrates the gyroscope by calculating the static offsets.
 * @details Takes 200 samples over a period of ~1 second while the sensor is
 * stationary to determine the zero-rate offset for the X, Y, and Z axes.
 * @return  int8_t Returns 0 when calibration is complete.
 */
int8_t ICM20948_gyro_Calibrate(void);

/**
 * @brief   Reads the raw accelerometer data and scales it to physical values (g).
 * @param   data Pointer to the ICM_Data_t structure to store the results.
 * @return  None
 */
void ICM20948_accel(ICM_Data_t *data);

/**
 * @brief   Reads the raw gyroscope data, applies offsets, and scales to °/s.
 * @param   data Pointer to the ICM_Data_t structure to store the results.
 * @return  None
 */
void ICM20948_gyro(ICM_Data_t *data);

/**
 * @brief   Reads the raw temperature data and converts it to °C.
 * @param   data Pointer to the ICM_Data_t structure to store the results.
 * @return  None
 */
void ICM20948_temp(ICM_Data_t *data);

/**
 * @brief   Computes roll and pitch angles using a complementary filter.
 * @details Fuses the high-frequency data from the gyroscope (integrated over time)
 * with the low-frequency absolute reference from the accelerometer to provide
 * stable, drift-free orientation angles.
 * @param   raw_data Pointer to the structure containing current sensor readings.
 * @param   angles Pointer to the structure where the calculated angles are stored.
 * @param   dt The time step (delta time) in seconds since the last calculation.
 * @return  None
 */
void ICM20948_Compute_Angles(ICM_Data_t *raw_data, ICM_Angles_t *angles, float dt);

#endif /* INC_ICM20948_H_ */
