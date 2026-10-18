// Modbus register map of the field controller (used by env:node-modbus).
//
// PLACEHOLDER MAP: these addresses are an assumption for bench testing with a
// PC slave simulator. The real map must be taken from the controller's manual
// (or provided by the advisor / site operator) and entered here; nothing else
// in the firmware needs to change.
//
// Addresses are 0-based PDU addresses (holding register 40001 in "Modicon"
// notation = address 0). Use -1 for a value the device does not provide.
#pragma once

#define MODBUS_SLAVE_ID     1
#define MODBUS_BAUD         9600   // 8N1

// --- Holding registers read for telemetry (function 0x03) ---
#define MODBUS_REG_LEVEL_MM       0   // tank level, mm
#define MODBUS_REG_VALVE_STATE    1   // 0 = closed, 1 = open, 2 = moving, 3 = fault
#define MODBUS_REG_PRESSURE_CBAR  2   // pressure, centibar
#define MODBUS_REG_FLOW_DL_MIN    3   // flow, decilitres per minute
#define MODBUS_REG_ALARMS         4   // bit 0 overflow, bit 1 dry run, bit 2 sensor fault

// --- Holding registers written for commands (function 0x06) ---
#define MODBUS_REG_VALVE_CMD      10  // write 1 = open, 0 = close
#define MODBUS_REG_ENABLE         11  // write 1 = enabled, 0 = disabled
