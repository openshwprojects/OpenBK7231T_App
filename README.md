# OpenBeken (OpenBK7231T_App)

OpenBeken is a powerful, open-source firmware for Tuya-class IoT devices, providing
a Tasmota/ESPHome-like experience across a wide range of Wi-Fi and Wi-Fi+BLE chipsets.

It supports MQTT, Home Assistant integration, OTA updates, scripting, custom drivers,
and extensive GPIO/peripheral control — without relying on vendor cloud services.

> ⚠️ Repository name note  
> Although this repository is named **OpenBK7231T_App**, it has evolved into a
> **multi-platform firmware** supporting dozens of chip families beyond BK7231.

---

## Who This Project Is For

- **End users** flashing Tuya-based devices with prebuilt firmware
- **Home Assistant / MQTT users** replacing vendor firmware
- **Advanced users** building custom firmware images
- **Developers** adding drivers, platforms, or SDK support
- **Reverse engineers** analysing vendor SDKs and firmware layouts

If you only want to flash a device, you do **not** need to build from source.

---

## Start Here (Choose Your Path)

- 👉 **Flash a device quickly**  
  See: [Flashing Guide](FLASHING.md) and [Releases](https://github.com/openshwprojects/OpenBK7231T_App/releases)

- 👉 **Check if your device is supported**  
  See: [Supported Devices & Templates](https://openbekeniot.github.io/webapp/devicesList.html)

- 👉 **Configure MQTT / Home Assistant**  
  See: [MQTT & HA Docs](docs/README.md)

- 👉 **Build firmware yourself**  
  See: [BUILDING.md](BUILDING.md)

- 👉 **Develop drivers or platforms**  
  See: [Developer Guide](docs/DEVELOPERS.md)

---

## Supported Platforms (Overview)

OpenBeken supports **30+ chip families** across multiple vendors.

A full, detailed list (with module examples and flashing notes) is maintained here:

➡️ **[SUPPORTED_CHIPS.md](SUPPORTED_CHIPS.md)**

### Major Families Include

- **Beken** (BK7231T/N/M/S/U, BK7238, BK7252)
- **Bouffalo Lab / LeapFive** (BL602, TG7100C, LF686)
- **WinnerMicro** (W600/W601/W800/W801/W803)
- **Espressif** (ESP8266, ESP8285, ESP32, ESP32-S2/S3/C2/C3/C6)
- **Realtek Ameba** (RTL8710, RTL8720, RTL87xx)
- **Xradio / Allwinner** (XR809, XR806, XR872)
- **Lightning Semi** (LN882H)
- **Transa / ESWIN** (TR6260, ECR6600)
- **Others** (TXW81X, RDA5981)

---

## Platform Support Status

- **Stable / Widely Used** – Most Beken, BL602, ESP32, WinnerMicro, Realtek platforms
- **Functional but Evolving** – LN882H, TR6260, ECR6600
- **In Progress / Experimental** – XR872, BK7252U/N

See: [Platform Status](docs/PLATFORM_STATUS.md)

---

## Features

OpenBeken provides a consistent feature set across platforms where hardware allows.

### Core Firmware Capabilities
- Tasmota-style web UI and console
- OTA firmware upgrades (platform-dependent)
- LittleFS support for scripts and web assets
- Command-based configuration system
- Startup command storage (autoexec)

### Home Automation
- MQTT with Home Assistant (YAML + HA Discovery)
- Tasmota-compatible TELE/STAT/CMND topics
- ioBroker compatibility
- REST API and custom web dashboards

### Hardware & Drivers
- GPIO mapping and automatic Tuya GPIO extraction
- Relay, PWM, button, LED, and input handling
- TuyaMCU support (including battery-powered protocols)
- I2C (HW/SW): AHT, BME, SHT, MCP23017, PCF8574
- SPI (HW/SW): BL0942SPI and others
- Power metering: BL0937, BL0942
- LED drivers: SM2135, BP5758, SM15155
- WS2812B / addressable LEDs
- Shutters, motors, and bridge drivers

### Scripting & Automation
- Advanced event and rules system
- Timers, NTP-based scheduling
- Berry scripting (optional)
- DDP lighting protocol (xLights)

### Power Management
- Deep sleep with GPIO/timer wakeup
- Hybrid power-save modes
- Battery-optimised TuyaMCU devices

### Advanced / Specialist
- On-device charts and visualisation
- Windows simulator mode
- Matter device conversion
- Custom driver framework

---

## Prebuilt Firmware & Online Builds

- ✅ Automatically compiled binaries available via **Releases**
- ✅ Per-user configurable online build system
- ✅ Docker-based build support

See:
- [Online Builds Guide](https://www.elektroda.com/rtvforum/viewtopic.php?p=20946719#20946719)
- [Docker Builds](docker/)

---

## Flashing

### Beken / BL602 / Common Platforms
- Use **BK7231GUIFlashTool**
- See: [FLASHING.md](FLASHING.md)

### Other Platforms
- Platform-specific tools available here:  
  https://github.com/openshwprojects/FlashTools

### Special Cases
- TXW81X requires CK-Link or STM32F103
- RTL8711AM requires JTAG or SPI (no UART flashing)

---

## Documentation Index

➡️ **[docs/README.md](docs/README.md)**

Includes:
- MQTT topics
- Console commands
- Flags and constants
- Pin roles and channel types
- autoexec examples
- FAQ

---

## Device Database & Community

- 📋 Interactive device/template database (700+ entries):  
  https://openbekeniot.github.io/webapp/devicesList.html

- 🧩 Submit teardowns and get templates added  
  https://www.elektroda.com/rtvforum/forum390.html

---

## Video Guides

- YouTube channel: https://www.youtube.com/@elektrodacom
- Flashing guides playlist
- Setup tips & tricks

---

## Further Reading (SDK & Reverse Engineering)

- https://www.elektroda.com/rtvforum/topic3850712.html
- https://www.elektroda.com/rtvforum/topic3866123.html
- https://www.elektroda.com/rtvforum/topic3806769.html

---

## Support the Project

If you find OpenBeken useful, consider supporting development:

https://www.paypal.com/paypalme/openshwprojects

Special thanks to all open-source contributors and the Elektroda community.
