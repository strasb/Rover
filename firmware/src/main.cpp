// ============================================================
//  LILYGO T-2CAN (ESP32-S3) – ODrive velocity control via CAN
//
//  CAN interface : MCP2515 (SPI) on CANA port
//  ODrive nodes  : 10 – 13  (v0.5.1 firmware)
//  Bitrate       : 250 kbps
//  Loop rate     : 100 Hz
//
//  CAN-ID format : (node_id << 5) | command_id
//  Commands used:
//    0x007 – Set axis requested state
//    0x00B – Set controller modes
//    0x00D – Set input velocity
//
//  Payload encoding: float32 little-endian
// ============================================================

#include <Arduino.h>
#include <SPI.h>
#include <mcp_can.h>

#include "pin_config.h"

// ── ODrive command IDs ────────────────────────────────────────
static constexpr uint8_t CMD_SET_AXIS_STATE    = 0x07;
static constexpr uint8_t CMD_SET_CTRL_MODE     = 0x0B;
static constexpr uint8_t CMD_SET_INPUT_VEL     = 0x0D;

// ── ODrive axis / controller mode constants ───────────────────
static constexpr uint32_t AXIS_STATE_IDLE             = 1;
static constexpr uint32_t AXIS_STATE_CLOSED_LOOP      = 8;
static constexpr uint32_t CTRL_MODE_VELOCITY_CONTROL  = 2;
static constexpr uint32_t INPUT_MODE_VEL_RAMP          = 2;

// ── MCP2515 instance ─────────────────────────────────────────
MCP_CAN can(MCP2515_PIN_CS);

// ── Timing ───────────────────────────────────────────────────
static uint32_t lastSendMs   = 0;
static uint32_t lastActiveMs = 0;

// ── Desired velocity per node (index 0 = node 10) ────────────
static float nodeVelocity[4] = {0.0f, 0.0f, 0.0f, 0.0f};
static bool  nodeEnabled[4]  = {false, false, false, false};

// ─────────────────────────────────────────────────────────────
//  Helpers
// ─────────────────────────────────────────────────────────────

/** Build the 11-bit standard CAN ID used by ODrive v0.5. */
static inline uint32_t odriveCanId(uint8_t nodeId, uint8_t cmd)
{
    return ((uint32_t)nodeId << 5) | cmd;
}

/** Pack a float32 into a byte buffer (little-endian). */
static void packFloat(uint8_t *buf, float val)
{
    memcpy(buf, &val, 4);
}

/** Pack a uint32 into a byte buffer (little-endian). */
static void packU32(uint8_t *buf, uint32_t val)
{
    buf[0] = val & 0xFF;
    buf[1] = (val >> 8) & 0xFF;
    buf[2] = (val >> 16) & 0xFF;
    buf[3] = (val >> 24) & 0xFF;
}

/** Send an 8-byte CAN frame. Returns true on success. */
static bool canSend(uint32_t canId, const uint8_t *data, uint8_t len)
{
    byte result = can.sendMsgBuf(canId, 0 /*standard*/, len, (byte *)data);
    return (result == CAN_OK);
}

// ─────────────────────────────────────────────────────────────
//  ODrive command helpers
// ─────────────────────────────────────────────────────────────

/** Set axis requested state (8 bytes: state u32 + padding). */
static void odriveSetAxisState(uint8_t nodeId, uint32_t state)
{
    uint8_t buf[8] = {0};
    packU32(buf, state);
    canSend(odriveCanId(nodeId, CMD_SET_AXIS_STATE), buf, 8);
}

/**
 * Set controller + input mode.
 * Bytes 0-3: control_mode (u32), Bytes 4-7: input_mode (u32).
 */
static void odriveSetControllerMode(uint8_t nodeId,
                                    uint32_t ctrlMode,
                                    uint32_t inputMode)
{
    uint8_t buf[8] = {0};
    packU32(buf,     ctrlMode);
    packU32(buf + 4, inputMode);
    canSend(odriveCanId(nodeId, CMD_SET_CTRL_MODE), buf, 8);
}

/**
 * Set input velocity.
 * Bytes 0-3: velocity (float32), Bytes 4-7: torque_ff (float32, 0.0).
 */
static void odriveSetVelocity(uint8_t nodeId, float vel)
{
    // Clamp to safe range
    vel = constrain(vel, -MAX_VELOCITY_TURNS_S, MAX_VELOCITY_TURNS_S);

    uint8_t buf[8] = {0};
    packFloat(buf,     vel);
    packFloat(buf + 4, 0.0f); // torque feed-forward = 0
    canSend(odriveCanId(nodeId, CMD_SET_INPUT_VEL), buf, 8);
}

// ─────────────────────────────────────────────────────────────
//  Enable / disable all nodes
// ─────────────────────────────────────────────────────────────

static void enableAllNodes()
{
    for (uint8_t n = ODRIVE_NODE_MIN; n <= ODRIVE_NODE_MAX; n++)
    {
        uint8_t idx = n - ODRIVE_NODE_MIN;
        odriveSetControllerMode(n,
                                CTRL_MODE_VELOCITY_CONTROL,
                                INPUT_MODE_VEL_RAMP);
        delay(5);
        odriveSetAxisState(n, AXIS_STATE_CLOSED_LOOP);
        delay(5);
        nodeEnabled[idx] = true;
    }
    lastActiveMs = millis();
    Serial.println("[ODrive] All nodes enabled (closed-loop velocity)");
}

static void disableAllNodes()
{
    for (uint8_t n = ODRIVE_NODE_MIN; n <= ODRIVE_NODE_MAX; n++)
    {
        uint8_t idx = n - ODRIVE_NODE_MIN;
        odriveSetVelocity(n, 0.0f);
        delay(2);
        odriveSetAxisState(n, AXIS_STATE_IDLE);
        nodeEnabled[idx] = false;
    }
    Serial.println("[ODrive] All nodes disabled (idle)");
}

// ─────────────────────────────────────────────────────────────
//  Velocity commands (example API – adapt to your motion stack)
// ─────────────────────────────────────────────────────────────

/**
 * Set desired velocity for a single ODrive node.
 * nodeId must be in [ODRIVE_NODE_MIN, ODRIVE_NODE_MAX].
 */
void setNodeVelocity(uint8_t nodeId, float vel)
{
    if (nodeId < ODRIVE_NODE_MIN || nodeId > ODRIVE_NODE_MAX) return;
    nodeVelocity[nodeId - ODRIVE_NODE_MIN] = vel;
    lastActiveMs = millis();
}

// ─────────────────────────────────────────────────────────────
//  Control loop (called at SAMPLE_RATE_HZ)
// ─────────────────────────────────────────────────────────────

static void controlLoop()
{
    uint32_t now = millis();

    // Watchdog: if no velocity update for ODRIVE_TIMEOUT_MS, zero all nodes
    if (now - lastActiveMs > ODRIVE_TIMEOUT_MS)
    {
        for (uint8_t n = ODRIVE_NODE_MIN; n <= ODRIVE_NODE_MAX; n++)
            nodeVelocity[n - ODRIVE_NODE_MIN] = 0.0f;
    }

    // Send velocity to each enabled node
    for (uint8_t n = ODRIVE_NODE_MIN; n <= ODRIVE_NODE_MAX; n++)
    {
        uint8_t idx = n - ODRIVE_NODE_MIN;
        if (nodeEnabled[idx])
            odriveSetVelocity(n, nodeVelocity[idx]);
    }
}

// ─────────────────────────────────────────────────────────────
//  setup / loop
// ─────────────────────────────────────────────────────────────

void setup()
{
    Serial.begin(115200);
    delay(500);
    Serial.println("[Boot] LILYGO T-2CAN ODrive controller");

    // ── Configure MCP2515 SPI pins ───────────────────────────
    SPI.begin(MCP2515_PIN_SCK, MCP2515_PIN_MISO, MCP2515_PIN_MOSI, MCP2515_PIN_CS);

    // ── Initialise MCP2515 ───────────────────────────────────
    while (can.begin(MCP_ANY, CAN_BITRATE, MCP2515_OSC) != CAN_OK)
    {
        Serial.println("[CAN] MCP2515 init failed – retrying in 1 s …");
        delay(1000);
    }
    can.setMode(MCP_NORMAL);
    Serial.println("[CAN] MCP2515 ready at 250 kbps");

    // ── Enable ODrive nodes ──────────────────────────────────
    enableAllNodes();

    lastSendMs   = millis();
    lastActiveMs = millis();
}

void loop()
{
    uint32_t now = millis();

    // ── 100 Hz control loop ──────────────────────────────────
    if (now - lastSendMs >= SAMPLE_PERIOD_MS)
    {
        lastSendMs = now;
        controlLoop();
    }

    // ── Incoming CAN frames (optional monitoring) ────────────
    if (can.checkReceive() == CAN_MSGAVAIL)
    {
        long     rxId  = 0;
        uint8_t  rxLen = 0;
        uint8_t  rxBuf[8];
        can.readMsgBuf(&rxId, &rxLen, rxBuf);
        // TODO: parse ODrive heartbeat / encoder feedback here
    }

    // ── Serial command interface (simple demo) ───────────────
    // Send:  <nodeId>,<velocity>\n   e.g. "10,0.5\n"
    if (Serial.available())
    {
        String line = Serial.readStringUntil('\n');
        line.trim();
        int comma = line.indexOf(',');
        if (comma > 0)
        {
            uint8_t nodeId = (uint8_t)line.substring(0, comma).toInt();
            float   vel    = line.substring(comma + 1).toFloat();
            setNodeVelocity(nodeId, vel);
            Serial.printf("[CMD] node %u → %.3f turns/s\n", nodeId, vel);
        }
        else if (line == "stop")
        {
            disableAllNodes();
        }
        else if (line == "start")
        {
            enableAllNodes();
        }
    }
}
