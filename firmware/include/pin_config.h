#pragma once

// ============================================================
//  Board: LILYGO T-2CAN  (ESP32-S3)
//  CAN-A port: MCP2515 via SPI
// ============================================================

// ── CAN mode ─────────────────────────────────────────────────
// T_2Can   → MCP2515 (standard CAN 2.0B) on the CANA port
// T_2Can_Fd → CAN-FD controller (CANB, built-in TWAI)
#define CAN_MODE T_2Can

// ── MCP2515 SPI wiring (CANA port, ESP32-S3 default) ─────────
// Verify against your specific board revision / schematic.
#define MCP2515_PIN_SCK  12
#define MCP2515_PIN_MOSI 11
#define MCP2515_PIN_MISO 13
#define MCP2515_PIN_CS   10
// INT pin is defined for future interrupt-driven RX; polling is used by default.
#define MCP2515_PIN_INT   9

// ── MCP2515 oscillator crystal ───────────────────────────────
// Typical T-2CAN crystal is 8 MHz; change to MCP_16MHZ if needed.
#define MCP2515_OSC MCP_8MHZ

// ── CAN bus bitrate ──────────────────────────────────────────
#define CAN_BITRATE CAN_250KBPS

// ── ODrive node IDs (four boards) ───────────────────────────
#define ODRIVE_NODE_MIN 10
#define ODRIVE_NODE_MAX 13

// ── Velocity / acceleration limits ──────────────────────────
#define MAX_VELOCITY_TURNS_S   1.0f   // turns / s
#define MAX_ACCEL_TURNS_S2     0.5f   // turns / s²

// ── Control-loop timing ─────────────────────────────────────
#define SAMPLE_RATE_HZ      100       // Hz
#define SAMPLE_PERIOD_MS    (1000 / SAMPLE_RATE_HZ)  // 10 ms
#define ODRIVE_TIMEOUT_MS   250       // watchdog / idle timeout
