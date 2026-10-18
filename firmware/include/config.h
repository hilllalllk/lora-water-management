// Shared hardware and timing settings.
// Pin numbers are for the ESP32 DevKit v1; for other boards adjust them per docs/04-donanim.md.
#pragma once

#include <stdint.h>

// --- E32 wiring (UART2) ---
#define PIN_E32_RX   16   // ESP32 RX  <- E32 TXD
#define PIN_E32_TX   17   // ESP32 TX  -> E32 RXD
#define PIN_E32_M0   4
#define PIN_E32_M1   5
#define PIN_E32_AUX  18

// --- Prototype field device (end node only) ---
#define PIN_VALVE_LED  2   // on-board LED
#define PIN_LEVEL_ADC  34  // potentiometer wiper

// --- Modbus RTU field device over RS-485 (env:node-modbus, UART1 + MAX3485) ---
#define PIN_RS485_RX  25   // ESP32 RX  <- MAX3485 RO
#define PIN_RS485_TX  26   // ESP32 TX  -> MAX3485 DI
#define PIN_RS485_DE  27   // MAX3485 DE and /RE tied together (HIGH = transmit)

// --- Radio ---
#define LORA_CHANNEL 0x17  // 433 MHz

// --- Timing ---
#define TELEMETRY_PERIOD_S_DEFAULT 60
#define TELEMETRY_JITTER_PERCENT   20     // ±20% random jitter to reduce collisions
#define HEARTBEAT_PERIOD_S         300
#define ACK_TIMEOUT_MS             2000
#define MAX_TRIES                  3
#define FRAME_GAP_MS               50

// The end-node address is set in platformio.ini via -DNODE_ADDRESS=0x1001.
#ifndef NODE_ADDRESS
#define NODE_ADDRESS 0x1001
#endif

#define FIRMWARE_VERSION "0.1.0"
