# STM32L4-BareMetal-Avionics ✈️

> A bare-metal, RTOS-free flight instrument and telemetry logger for the STM32L4 series. Features 9-DoF AHRS sensor fusion, ARINC 429 transmission, and high-efficiency raw-sector SD logging.

**STM32L4-BareMetal-Avionics** is a complete, register-level flight instrumentation and data logging system built entirely from scratch. 

Written entirely without the STM32 Hardware Abstraction Layer (HAL) or a Real-Time Operating System (RTOS), this project demonstrates highly efficient, deterministic hardware control using custom peripheral drivers and strict hardware-timer orchestration. It acts as an Attitude and Heading Reference System (AHRS), calculating real-time pitch and roll, broadcasting telemetry over an ARINC 429 avionics bus, and logging flight data directly to physical SD card sectors.

---

## 🚀 Key Engineering Highlights

* **100% Bare-Metal Architecture:** Direct memory-mapped register manipulation for maximum execution speed, minimal RAM footprint, and total hardware transparency.
* **Deterministic Super-Loop:** Replaces heavy RTOS context-switching with precise hardware-timer interrupts (50Hz sensor polling, 10Hz UI refresh).
* **Sensor Fusion (AHRS):** Integrates an ICM-20948 9-DoF IMU using a custom complementary filter to calculate stable, drift-free pitch and roll angles.
* **ARINC 429 Databus:** Implements bit-banged High-Speed (100 kbps) ARINC 429 encoding and transmission with strict Return-to-Zero (RZ) timing protocols.
* **Raw-Sector SD Logging:** Bypasses heavy FAT32 file systems entirely. Uses a custom SPI driver to buffer and write data directly to raw 512-byte physical sectors, massively reducing MCU overhead and flash wear.
* **Custom Peripheral Drivers:** Includes scratch-built drivers for I2C (Repeated Start support), SPI (FIFO-aware), USART (`printf` retargeting), ADC, and advanced Timers (PWM servo control).

---

## ⚙️ Hardware & Pinout Configuration

This project was built for the **STM32L432 / STM32L412** architecture running at **80MHz**.

### Supported Peripherals
* **IMU:** InvenSense ICM-20948 (9-DoF)
* **Barometer:** Bosch BMP280
* **Analog Temp:** LM35 (Backup Temperature)
* **Display:** 1.3" OLED SH1106 (128x64)
* **Storage:** Generic SPI MicroSD Card Module
* **Actuation:** Standard 50Hz PWM Servo (Pitch Axis Control)

### Wiring Guide
| STM32 Pin | Function | Peripheral / Connection |
| :--- | :--- | :--- |
| **PA0** | TIM2_CH1 | PWM Output to Pitch Servo (50Hz) |
| **PA1** | GPIO OUT | ARINC 429 TXA (High) |
| **PA2** | USART2_TX | Serial Debug TX (115200 Baud) |
| **PA4** | GPIO OUT | SPI1 Chip Select (CS) for SD Card |
| **PA8** | GPIO OUT | ARINC 429 TXB (Low) |
| **PA12** | GPIO OUT | Pitch Exceedance Warning LED |
| **PA15** | USART2_RX | Serial Debug RX |
| **PB0** | ADC1_IN15 | LM35 Analog Temperature Sensor |
| **PB1** | GPIO OUT | Roll Exceedance Warning LED |
| **PB3** | SPI1_SCK | SD Card Clock |
| **PB4** | SPI1_MISO | SD Card MISO |
| **PB5** | SPI1_MOSI | SD Card MOSI |
| **PB6** | I2C1_SCL | I2C Clock (IMU, BMP280, OLED) |
| **PB7** | I2C1_SDA | I2C Data (IMU, BMP280, OLED) |

---

## 📁 Repository Structure

```text
├── Core/
│   ├── Inc/
│   │   ├── ADC.h          # Bare-metal ADC1 configuration
│   │   ├── ARINC429.h     # ARINC encoding and RZ bit-banging
│   │   ├── BMP280.h       # I2C Barometer driver with Bosch compensation
│   │   ├── I2C.h          # Bare-metal I2C1 driver (blocking)
│   │   ├── ICM20948.h     # I2C IMU driver & Complementary Filter
│   │   ├── Rcc_Config.h   # 80MHz PLL and Flash latency setup
│   │   ├── SD.h           # Raw block SPI SD Card driver
│   │   ├── SH1106.h       # OLED display driver & 5x8 font
│   │   ├── SPI.h          # Bare-metal SPI1 driver (dynamic speed)
│   │   ├── Timer.h        # TIM2 (PWM), TIM6 (50Hz), TIM7 (10Hz) setup
│   │   ├── USART.h        # USART2 driver and _write syscall redirect
│   │   └── delay.h        # DWT cycle-counter microsecond delays
│   └── Src/
│       ├── main.c         # Super-loop and interrupt orchestration
│       └── [Driver .c files]
└── [STM32CubeIDE Project Files]
