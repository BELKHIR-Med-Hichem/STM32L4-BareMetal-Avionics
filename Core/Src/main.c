/**
 * @file    main.c
 * @author  Meh
 * @date    May 2026
 * @brief   Main application program for the STM32L432 bare-metal flight instrument.
 * @details This file serves as the system orchestrator. It initializes all hardware
 * peripherals and drivers, establishes a deterministic super-loop architecture using
 * hardware timer interrupts, and handles sensor fusion, ARINC 429 transmission,
 * servo actuation, OLED display updates, and bare-metal SD card data logging.
 */

#include "Rcc_Config.h"
#include "delay.h"
#include "USART.h"
#include "I2C.h"
#include "SPI.h"
#include "ADC.h"
#include "Timer.h"

#include "BMP280.h"
#include "ICM20948.h"
#include "SH1106.h"
#include "SD.h"

#include <stdio.h>
#include <string.h>
#include "ARINC429.h"

/** @defgroup Global_Variables System Data Structures
 * @{
 */
BMP280_Data_t   myEnv;                  /**< Environmental data (Pressure, Temperature) */
ICM_Data_t      myAccel;                /**< Raw 9-DoF IMU data */
ICM_Angles_t    myAngles = {0.0f, 0.0f};/**< Filtered orientation angles (Pitch, Roll) */
uint32_t        arinc_word;             /**< Encoded 32-bit ARINC 429 transmission word */
/** @} */

/* ================================================================
 * Hardware Timer Interrupt Service Routines (ISRs)
 * ================================================================ */

/**
 * @brief Flag set by TIM6 at 50Hz to trigger the high-speed sensor reading loop.
 */
volatile uint8_t time_to_sample = 0;

/**
 * @brief   TIM6 Interrupt Service Routine (50 Hz / 20ms).
 * @details Clears the update interrupt flag and sets the time_to_sample software flag.
 */
void TIM6_DAC_IRQHandler(void) {
    if (TIM6->SR & 0x01U) {
        TIM6->SR &= ~0x01U; /* Clear Update Interrupt Flag */
        time_to_sample = 1; /* Signal main loop to execute sample block */
    }
}

/**
 * @brief Flag set by TIM7 at 10Hz to trigger the low-speed display/serial loop.
 */
volatile uint8_t time_to_display = 0;

/**
 * @brief   TIM7 Interrupt Service Routine (10 Hz / 100ms).
 * @details Clears the update interrupt flag and sets the time_to_display software flag.
 */
void TIM7_IRQHandler(void) {
    if (TIM7->SR & 0x01U) {
        TIM7->SR &= ~0x01U; /* Clear Update Interrupt Flag */
        time_to_display = 1;/* Signal main loop to execute display block */
    }
}

/* ================================================================
 * LED Configuration
 * ================================================================ */

/**
 * @brief   Configures GPIO pins for the status indicator LEDs.
 * @details Sets up PA12 (used for pitch warning) and PB1 (used for roll warning)
 * as general-purpose push-pull outputs.
 * @return  None
 */
static void LED_Config(void) {
    /* Enable clocks for GPIOA and GPIOB */
    RCC->AHB2ENR  |= RCC_AHB2ENR_GPIOAEN | RCC_AHB2ENR_GPIOBEN;

    /* Set PA12 and PB1 to Output Mode (01) */
    GPIOA->MODER  &= ~(3U << (12 * 2)); GPIOA->MODER  |= (1U << (12 * 2));
    GPIOB->MODER  &= ~(3U <<  (1 * 2)); GPIOB->MODER  |= (1U <<  (1 * 2));

    /* Set Output Type to Push-Pull */
    GPIOA->OTYPER &= ~(1U << 12);       GPIOB->OTYPER &= ~(1U << 1);

    /* Set Output Speed to High */
    GPIOA->OSPEEDR|= (3U << (12 * 2));  GPIOB->OSPEEDR|= (3U << (1 * 2));
}

/* ================================================================
 * Main Application Entry Point
 * ================================================================ */

/**
 * @brief   Main execution loop.
 * @details Initializes all hardware, performs sensor setup and calibration,
 * writes the SD card CSV header, and enters an infinite polling loop that
 * dispatches tasks deterministically based on hardware timer flags.
 * @return  int Never returns.
 */
int main(void)
{
    /* ---- Hardware Initialization ---- */
    Sys_CLK_Config();   // 1. Core clock to 80MHz
    DWT_Init();         // 2. Microsecond delay timer
    USART2_Config();    // 3. Serial comms (printf support)
    I2C1_Config();      // 4. I2C Bus (OLED, IMU, BMP280)
    SPI_Init();         // 5. SPI Bus (SD Card)
    PWM_Init();         // 6. Servo control output
    ADC1_Init();        // 7. Analog read for LM35
    ARINC429_Init();    // 8. Avionics databus GPIO pins
    Sensor_Timer();     // 9. 50Hz Timer
    Display_Timer();    // 10. 10Hz Timer
    LED_Config();       // 11. Warning LEDs

    delay_ms(100);
    OLED_Clear();

    /* ---- Sensor / Peripheral Initialization ---- */
    printf("Init Sensors...\r\n");
    if (BMP280_Init()  != 0) while(1); // Trap if barometer fails
    if (ICM20948_Init() != 0) while(1); // Trap if IMU fails
    SD_Init();

    /* ---- Write a CSV header into the first sector ---- */
    const char *header = "pressure_hPa,temperature_C,lm35_temp_C,pitch_deg,roll_deg\r\n";
    SD_Log(header, strlen(header));
    /* The header will be flushed automatically once the first 512-byte
       block fills. No need to flush manually here unless you want to
       guarantee it lands before the first data arrives. */

    /* ---- Display Ready Status ---- */
    OLED_Init();
    OLED_Clear();
    OLED_SetCursor(0, 0);
    OLED_PrintString("Sensors OK!");
    delay_ms(1000);
    OLED_Clear();

    /* ================================================================
     * Infinite Super-Loop
     * ================================================================ */
    while (1)
    {
        /* ---- 50 Hz (20ms) Sampling & Control Block ---- */
        if (time_to_sample == 1) {
            time_to_sample = 0;

            /* -- 1. Sensor reads -- */
            BMP280_Temp(&myEnv);
            BMP280_Pressure(&myEnv);
            ICM20948_gyro(&myAccel);
            ICM20948_accel(&myAccel);
            ICM20948_temp(&myAccel);

            /* Apply complementary filter with dt = 0.02 seconds (50Hz) */
            ICM20948_Compute_Angles(&myAccel, &myAngles, 0.02f);

            /* -- 2. ARINC 429 Transmission -- */
            arinc_word = ARINC429_Encode(myEnv.pressure);
            ARINC429_Transmit(arinc_word);

            /* -- 3. Servo Actuation (Map pitch to 1000..2000 µs PWM) -- */
            /* Cap pitch values to +/- 90 degrees to protect servo limits */
            if (myAngles.pitch < -90.0f) myAngles.pitch = -90.0f;
            if (myAngles.pitch >  90.0f) myAngles.pitch =  90.0f;

            /* Linear interpolation: 1500µs center, scaled by 500µs per 90 degrees */
            TIM2->CCR1 = (uint32_t)(1500.0f + myAngles.pitch * (500.0f / 90.0f));

            /* -- 4. Warning LEDs -- */
            /* Pitch Exceedance Warning (PA12) */
            if (myAngles.pitch > 40.0f || myAngles.pitch < -40.0f)
                GPIOA->BSRR = (1U << 12); // Turn ON
            else
                GPIOA->BSRR = (1U << 28); // Turn OFF (Bit 12 + 16 = 28)

            /* Roll Exceedance Warning (PB1) */
            if (myAngles.roll > 40.0f || myAngles.roll < -40.0f)
                GPIOB->BSRR = (1U << 1);  // Turn ON
            else
                GPIOB->BSRR = (1U << 17); // Turn OFF (Bit 1 + 16 = 17)

            /* ============================================================
             * 5. SD CARD LOGGING (Bare metal, no FatFS)
             *
             * Each call to SD_Log() copies the text into a 512-byte RAM
             * buffer. When the buffer is full, SD_Log() automatically
             * writes one physical sector to the card and resets the buffer.
             * This means a single SD_WriteBlock() SPI transaction covers
             * many samples, which is highly efficient and minimizes wear.
             * ============================================================ */
            char logLine[64];
            float temp_analogique = ADC1_ReadLM35();

            /* Format payload as CSV */
            int logLen = snprintf(logLine, sizeof(logLine),
                                   "%.1f,%.1f,%.1f,%.2f,%.2f\r\n",
                                   myEnv.pressure,
                                   myEnv.temperature,
                                   temp_analogique,
                                   myAngles.pitch,
                                   myAngles.roll);

            /* Buffer the line to SD */
            uint8_t sdStatus = SD_Log(logLine, (uint16_t)logLen);

            /* Show brief SD logging status on OLED row 6 */
            OLED_SetCursor(6, 0);
            if (sdStatus == 0)
                OLED_PrintString("LOG OK    ");
            else {
                char errBuf[16];
                snprintf(errBuf, sizeof(errBuf), "ERR:0x%02X  ", sdStatus);
                OLED_PrintString(errBuf);
            }
        }

        /* ---- 10 Hz (100ms) Display & Serial Block ---- */
        if (time_to_display == 1) {
            time_to_display = 0;

            /* Read analog temp again for UI (Note: consider caching the 50Hz read to save CPU) */
            float temp_analogique = ADC1_ReadLM35();

            /* -- 1. Serial Telemetry (Blocking UART) -- */
            printf("ARINC: 0x%08lX\r\n", arinc_word);
            printf("P:%.1f T:%.1f LM35:%.1f Pt:%.1f Rl:%.1f\r\n",
                   myEnv.pressure, myEnv.temperature,
                   temp_analogique,
                   myAngles.pitch, myAngles.roll);

            /* -- 2. OLED Display Updates -- */
            char displayBuffer[32];

            /* Row 0: Environment */
            snprintf(displayBuffer, sizeof(displayBuffer),
                     "T:%.1f P:%.0f  ", myEnv.temperature, myEnv.pressure);
            OLED_SetCursor(0, 0);
            OLED_PrintString(displayBuffer);

            /* Row 2: Analog Backup Temperature */
            snprintf(displayBuffer, sizeof(displayBuffer),
                     "T_ADC:%.2f ", temp_analogique);
            OLED_SetCursor(2, 0);
            OLED_PrintString(displayBuffer);

            /* Row 4: Attitude Data */
            snprintf(displayBuffer, sizeof(displayBuffer),
                     "Pt:%.1f Rl:%.1f  ", myAngles.pitch, myAngles.roll);
            OLED_SetCursor(4, 0);
            OLED_PrintString(displayBuffer);
        }
    }
}
