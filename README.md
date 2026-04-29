# GMpro87dev ESP32 WiFi Repeater

## Firmware Features
- WiFi Repeater / WISP Mode
- Modern Web UI with GMpro87dev branding
- Network Scanner
- Connected Clients Monitoring
- AP Settings Configuration
- TX Power Control

## Build with GitHub Actions

1. Upload folder ini ke GitHub repo lu
2. Buka Actions tab
3. Run workflow "Build ESP32 Firmware"
4. Download .bin dari Artifacts

## Or with Arduino IDE

1. Install ESP32 board via Board Manager
2. Buka `esp32_wifi_repeater.ino`
3. Compile & Upload

## Flash Command

```bash
esptool.py --chip esp32 --port COM3 --baud 460800 write_flash 0x1000 firmware.bin
```

## Web UI

Buka http://192.168.4.1 setelah flash

## Credits
- Based on ESP32 Arduino Core
- UI by GMpro87dev