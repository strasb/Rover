# LILYGO T-2CAN/T-CAN485 Setup Guide for ODrive Controllers

## Introduction
This guide provides the necessary steps and configurations to set up multiple ODrive controllers (v0.5.1 firmware) using the LILYGO T-2CAN/T-CAN485 board via CANA port with MCP2515 (SPI).

## Wiring
- **CANA CANH/CANL/GND**: Connect the CANH, CANL, and GND pins from the T-2CAN board to the respective pins on the ODrive boards. Ensure proper grounding to avoid communication issues.
- **Termination**: Use a 120-ohm resistor across CANH and CANL at the farthest device in the CAN network to prevent reflections.

## Configuration
### Required `pin_config.h` Change
- Enable `T_2Can` instead of `T_2Can_Fd`: 
  ```cpp
  #define CAN_MODE T_2Can
  ```  

### Node ID Scheme
- Use Node IDs 10 to 13 for the four ODrive boards.

### CAN ID Format
- The format for CAN ID is constructed as follows: 
  ```cpp
  CAN_ID = (node << 5) | command;
  ```

### Command IDs Used
- **0x007**: Set axis requested state  
- **0x00B**: Set controller modes  
- **0x00D**: Set input velocity

### Payload Formats
- Use `float32` LE (Little Endian) formats for sending data.

### Safe Values
- **Max Velocity**: 1.0 turns/s  
- **Max Acceleration**: 0.5 turns/s²  
- **Sample Rate**: 100 Hz  
- **Timeout**: 250 ms

## ODrive-Side Configuration Steps via USB
1. Connect the ODrive to the computer via USB.
2. Use the ODrive Tool to send commands based on the above configurations.

## PlatformIO Upload Steps
1. Create a new PlatformIO project for your ODrive.
2. Configure the `platformio.ini` with the required libraries.
3. Upload the code via PlatformIO.

## Troubleshooting Checklist
- **Bitrate Mismatch**: Ensure all devices are set to the same baud rate (250 kbps).
- **Ground Issues**: Verify all components have a common ground connection.
- **Termination**: Check for the 120-ohm resistor on the CAN bus.
- **BOOT-0 Upload**: Verify that the correct firmware is uploaded to the ODrive.

## Appendix
### Sample Code for src/main.cpp (CANA MCP2515)
```cpp
#include <MCP2515.h>

MCP2515 can; // Initialize CAN controller

void setup() {
  can.begin(); // Start CAN controller
}

void loop() {
  // Implement velocity control scaffold here
  // Send required CAN frames
}
```

## Relevant Repositories and Files
- ODrive Firmware: https://github.com/madcowswe/ODrive
- MCP2515 Library: https://github.com/coryjfowler/MCP_CAN_lib


