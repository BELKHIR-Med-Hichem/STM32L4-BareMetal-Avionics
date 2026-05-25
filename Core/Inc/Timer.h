/**
 * @file    Timer.h
 * @author  Meh
 * @date    May 9, 2026
 * @brief   Hardware Timer configuration driver for the STM32L432xx.
 * @details This header defines the initialization functions for configuring
 * the internal hardware timers. It includes a PWM generator (TIM2) and
 * two time-base periodic interrupts (TIM6 and TIM7) used for sensor polling
 * and display updates.
 */

#ifndef INC_TIMER_H_
#define INC_TIMER_H_

/**
 * @brief   Initializes Timer 2 (TIM2) for PWM signal generation on PA0.
 * @details Sets up TIM2 Channel 1 to output a 50Hz (20ms) PWM signal,
 * which is standard for servo motor control. The initial pulse width
 * is set to 1.5ms (neutral position).
 * @return  None
 */
void PWM_Init(void);

/**
 * @brief   Initializes Timer 6 (TIM6) as a periodic interrupt timer for sensors.
 * @details Configures TIM6 to generate an update interrupt at 50Hz (every 20ms).
 * This provides a stable time-base for sampling physical sensors at a fixed rate.
 * @return  None
 */
void Sensor_Timer(void);

/**
 * @brief   Initializes Timer 7 (TIM7) as a periodic interrupt timer for the display.
 * @details Configures TIM7 to generate an update interrupt at 10Hz (every 100ms).
 * This provides a slower, stable time-base for refreshing the OLED screen without
 * overloading the CPU.
 * @return  None
 */
void Display_Timer(void);

#endif /* INC_TIMER_H_ */
