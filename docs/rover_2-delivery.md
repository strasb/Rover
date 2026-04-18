# Rover_2 Delivery Package (T-2CAN + 4x MKS ODrive MINI)

This project already contains the required firmware source and documentation for the LILYGO T-2CAN ESP32-S3 target using PlatformIO.

## 1) Files to place in `strasb/Rover_2` before compiling

Copy these paths into your Rover_2 repository:

- `firmware/platformio.ini`
- `firmware/src/main.cpp`
- `firmware/include/pin_config.h`
- `docs/t-2can-odrive-setup.md`
- `docs/rover_2-delivery.md`
- `.github/workflows/build-firmware.yml` (optional CI build)

## 2) Target behavior included

- Board: LILYGO T-2CAN (ESP32-S3, 16MB Flash, 8MB PSRAM)
- CAN interface: MCP2515 (CANA)
- Supported motor controllers: 4x MKS ODrive MINI
- Node ID range: `10..13`
- Bitrate: `250 kbps`
- Control loop: `100 Hz`

## 3) Build command (PlatformIO)

```bash
cd firmware
pio run --environment lilygo-t2can
```

## 4) Expected `.bin` output paths

- `firmware/.pio/build/lilygo-t2can/firmware.bin`
- `firmware/.pio/build/lilygo-t2can/bootloader.bin`
- `firmware/.pio/build/lilygo-t2can/partitions.bin`
