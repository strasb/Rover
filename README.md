# Rover

PlatformIO firmware for **LILYGO T-2CAN (ESP32-S3)** controlling **4x MKS ODrive MINI** nodes over CAN.

## Firmware target
- Board: ESP32-S3 (16MB Flash, 8MB OPI PSRAM)
- CAN controller: MCP2515 (T-2CAN CANA port)
- ODrive nodes: 10, 11, 12, 13

## Build (PlatformIO)
```bash
cd firmware
pio run --environment lilygo-t2can
```

## Output binaries
After a successful build:
- `firmware/.pio/build/lilygo-t2can/firmware.bin`
- `firmware/.pio/build/lilygo-t2can/bootloader.bin`
- `firmware/.pio/build/lilygo-t2can/partitions.bin`

## Setup and delivery docs
- `docs/t-2can-odrive-setup.md`
- `docs/rover_2-delivery.md`
