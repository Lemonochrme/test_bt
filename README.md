# Description

This project was a simple demo project for BLE capabilities of ESP32C3 through a simple "Hello World".

# Installation

Make sure you have platform io.

You have to configure the `sdkconfig.esp32-c3-devkitm-1` with the following :
```
CONFIG_IDF_TARGET="esp32c3"
CONFIG_BT_ENABLED=y
CONFIG_BT_BLE_42_FEATURES_SUPPORTED=y
```
Finally, build and run !

# Results

- Download nRF Connect app.
- Connect to ESP32 via bluetooth.
- Go to ESP32 Client
- Observe the "Hello World" in hex format under 0x4242 UUID : `48 65 6C 6C 6F 20 57 6F 72 6C 64`

![Screenshot_2025-03-11-23-33-49-019_no nordicsemi android mcp](https://github.com/user-attachments/assets/c0441bf6-e044-45a3-a5bd-93bea2d6b622)
