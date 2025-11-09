#ifndef PERIPHERAL_PINS_H
#define PERIPHERAL_PINS_H

/**
 * @file PeripheralPins.h
 * @brief Centralized pin definitions for all peripherals
 *
 * This file defines all GPIO pin assignments for the ESP32-S3 motor control system.
 * Pin assignments are carefully chosen to avoid conflicts with reserved pins
 * (USB, Flash, PSRAM) and to optimize peripheral routing.
 */

// ============================================================================
// DEPRECATED: Old motor control pins (merged to UART1)
// ============================================================================
// Motor control functionality has been merged into UART1 (GPIO 17, 18)
// These pins are now available for other uses
// #define PIN_MOTOR_PWM_OUTPUT        10  // DEPRECATED: Use UART1 PWM
// #define PIN_MOTOR_TACH_INPUT        11  // DEPRECATED: Use UART1 RPM

// ============================================================================
// DEBUG/TEST OUTPUT PINS
// ============================================================================
#define PIN_PWM_CHANGE_PULSE        12  // GPIO Output - Pulse on PWM parameter change (for glitch observation)

// ============================================================================
// STATUS LED
// ============================================================================
#define PIN_STATUS_LED              48  // WS2812 RGB status LED

// ============================================================================
// UART1 PINS (Unified Motor Control)
// ============================================================================
// UART1 - Multiplexable between UART and Motor Control (PWM/RPM) modes
// Motor control functions (previously on GPIO 10/11) are now on GPIO 17/18
#define PIN_UART1_TX                17  // UART1 TX / MCPWM PWM output (10Hz-500kHz)
#define PIN_UART1_RX                18  // UART1 RX / MCPWM Capture (RPM measurement, 1Hz-500kHz)

// UART2 - Standard UART (avoids GPIO36/37)
#define PIN_UART2_TX                43  // UART2 TX (2400-1.5Mbps)
#define PIN_UART2_RX                44  // UART2 RX (2400-1.5Mbps)

// ============================================================================
// PWM OUTPUT PINS
// ============================================================================
#define PIN_BUZZER_PWM              13  // LEDC CH0 - Passive buzzer (10Hz-20kHz)
#define PIN_LED_PWM                 14  // LEDC CH1 - LED brightness control

// ============================================================================
// DIGITAL OUTPUT PINS
// ============================================================================
#define PIN_RELAY_CONTROL           21  // GPIO Output - Relay control (HIGH active)
#define PIN_GPIO_OUTPUT             41  // GPIO Output - General purpose

// ============================================================================
// USER INPUT PINS (Active LOW with internal pull-up)
// ============================================================================
#define PIN_USER_KEY1               1   // User Key 1 - Duty/Frequency increase
#define PIN_USER_KEY2               2   // User Key 2 - Duty/Frequency decrease
#define PIN_USER_KEY3               42  // User Key 3 - Enter/Start (future use)

// ============================================================================
// RESERVED PINS (DO NOT USE)
// ============================================================================
// GPIO 19, 20: USB D-/D+ (USB OTG interface)
// GPIO 26-32: SPI Flash and PSRAM (critical for system operation)
// GPIO 36, 37: UART0 (RXD0/TXD0) - Used by USB CDC, avoid as requested

// ============================================================================
// PERIPHERAL CHANNEL ASSIGNMENTS
// ============================================================================

// LEDC Channels (for peripherals)
#define LEDC_CHANNEL_BUZZER         0   // Buzzer PWM
#define LEDC_CHANNEL_LED            1   // LED PWM

// LEDC Timers
#define LEDC_TIMER_BUZZER           0   // High-speed timer for buzzer
#define LEDC_TIMER_LED              1   // High-speed timer for LED

// MCPWM for UART1 Motor Control
// UART1 uses MCPWM for both PWM output and RPM capture (high precision, wide range)
#define MCPWM_UNIT_UART1_PWM        MCPWM_UNIT_1
#define MCPWM_TIMER_UART1_PWM       MCPWM_TIMER_0
#define MCPWM_GEN_UART1_PWM         MCPWM_OPR_A

#define MCPWM_UNIT_UART1_RPM        MCPWM_UNIT_0
#define MCPWM_CAP_UART1_RPM         MCPWM_SELECT_CAP1

// UART Numbers
#define UART_NUM_UART1              UART_NUM_1
#define UART_NUM_UART2              UART_NUM_2

#endif // PERIPHERAL_PINS_H
