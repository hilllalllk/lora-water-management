// Ortak donanım ve zamanlama ayarları.
// Pin numaraları ESP32 DevKit v1 içindir; farklı kartta docs/04-donanim.md'ye göre değiştirin.
#pragma once

#include <stdint.h>

// --- E32 bağlantısı (UART2) ---
#define PIN_E32_RX   16   // ESP32 RX  <- E32 TXD
#define PIN_E32_TX   17   // ESP32 TX  -> E32 RXD
#define PIN_E32_M0   4
#define PIN_E32_M1   5
#define PIN_E32_AUX  18

// --- Prototip saha cihazı (sadece uç düğüm) ---
#define PIN_VALVE_LED  2   // dahili LED
#define PIN_LEVEL_ADC  34  // potansiyometre orta ucu

// --- Radyo ---
#define LORA_CHANNEL 0x17  // 433 MHz

// --- Zamanlama ---
#define TELEMETRY_PERIOD_S_DEFAULT 60
#define TELEMETRY_JITTER_PERCENT   20     // çakışmaları azaltmak için ±%20 rastgele sapma
#define HEARTBEAT_PERIOD_S         300
#define ACK_TIMEOUT_MS             2000
#define MAX_TRIES                  3
#define FRAME_GAP_MS               50

// Uç düğüm adresi platformio.ini içinden -DNODE_ADDRESS=0x1001 ile verilir.
#ifndef NODE_ADDRESS
#define NODE_ADDRESS 0x1001
#endif

#define FIRMWARE_VERSION "0.1.0"
