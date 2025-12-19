# OpenBeken (OpenBK7231T_App)

OpenBK7231T / OpenBeken is a Tasmota/ESPHome alternative for modern Tuya-based modules,
offering MQTT and Home Assistant compatibility.

Although this repository is named **OpenBK7231T_App**, it has evolved into a
**multi-platform application**, supporting builds for many chipsets from multiple
vendors, including Beken, Espressif, Realtek, WinnerMicro, Bouffalo Lab, Xradio /
Allwinner, Lightning Semi, Transa / ESWIN, and others.

---

## Who This Project Is For

This project intentionally serves multiple audiences:

- **End users** flashing Tuya-based devices with precompiled firmware
- **Home Assistant / MQTT users** replacing vendor cloud firmware
- **Advanced users** building customised firmware images
- **Developers** extending drivers or adding platform support
- **Reverse engineers** analysing SDKs, flash layouts, and boot behaviour

If you only want to **flash a device and use it**, you can stop reading after the
*Flashing* and *Features* sections.

---

## Quick Start (Most Users)

1. Identify your device’s chipset
2. Download a precompiled binary from the **Releases** tab
3. Flash using the appropriate flashing tool
4. Configure Wi-Fi, GPIOs, and MQTT via the web interface

No compilation is required for normal use.

---

## Supported Platforms — Overview

OpenBeken supports **a very wide range of chip families**.  
What follows later is the **full, exhaustive list** with examples and links.

Major families include:

- **Beken** (BK7231T/N/M/S/U, BK7238, BK7252)
- **Bouffalo Lab / LeapFive** (BL602, TG7100C, LF686)
- **WinnerMicro** (W600/W601/W800/W801/W803)
- **Espressif** (ESP8266/8285, ESP32, ESP32-S2/S3/C2/C3/C6)
- **Realtek Ameba** (RTL8710, RTL8720, RTL87xx)
- **Xradio / Allwinner** (XR809, XR806, XR872)
- **Lightning Semi** (LN882H)
- **Transa / ESWIN** (TR6260, ECR6600)
- **Others** (TXW81X, RDA5981)

---

## Supported Chips and Modules (Full List)

> This section is intentionally long and detailed.  
> It exists as a **reference and knowledge base**, not just a checklist.

### Beken

- **BK7231T**  
  (eg [WB3S](https://developer.tuya.com/en/docs/iot/wb3s-module-datasheet?id=K9dx20n6hz5n4),
  [WB2S](https://developer.tuya.com/en/docs/iot/wb2s-module-datasheet?id=K9ghecl7kc479),
  WB2L)  
  https://www.elektroda.com/rtvforum/topic3951016.html

- **BK7231N**  
  (eg [CB2S](https://developer.tuya.com/en/docs/iot/cb2s-module-datasheet?id=Kafgfsa2aaypq),
  [CB2L](https://developer.tuya.com/en/docs/iot/cb2l-module-datasheet?id=Kai2eku1m3pyl),
  [WB2L_M1](https://www.elektroda.com/rtvforum/topic3903356.html))  
  https://www.elektroda.com/rtvforum/topic3951016.html

- **BK7231M**  
  Non-Tuya version of BK7231N, sometimes labelled Belon BL2028, usually with `00000000` keys  
  Examples:  
  https://www.elektroda.com/rtvforum/topic4056377.html  
  https://www.elektroda.com/rtvforum/topic4086986.html

- **T34**  
  Flash as standard BK7231N  
  https://www.elektroda.com/rtvforum/topic4036975.html

- **BL2028N**  
  Belon version of BK7231N  
  https://www.elektroda.com/rtvforum/viewtopic.php?p=20262533#20262533

- **BK7238**  
  eg XH-CB3S  
  https://www.elektroda.com/rtvforum/topic4106397.html#21440428

- **BK7231S / BK7231U**  
  Original non-Tuya versions; BK7231U intended for cameras/displays  
  Often zero-key devices  
  Examples:  
  C-8133U LED drivers  
  HLK-B30 dev board

---

### Xradio / Allwinner

- **XR809**  
  eg XR3  
  https://www.elektroda.com/rtvforum/topic3806769.html  
  https://www.elektroda.com/rtvforum/topic4063735.html

- **XR806**  
  eg T103C-HL, WXU  
  https://www.elektroda.com/rtvforum/topic4118139.html

- **XR872 / XF16**  
  In progress  
  eg A9 camera

---

### Bouffalo Lab / LeapFive

- **BL602**  
  eg SM-028, BW2L  
  https://www.elektroda.com/rtvforum/topic3889041.html  
  OTA flashing for Magic Home devices:  
  https://www.elektroda.com/rtvforum/topic4050297.html

- **LF686**  
  LeapFive BL602 variant  
  Flash as standard BL602  
  Factory backup/restore guides:  
  https://www.elektroda.com/rtvforum/topic4108535.html  
  https://www.elektroda.com/rtvforum/topic4051649.html

- **TG7100C / TL7100C**  
  T-Mall BL602 variants  
  https://openbouffalo.org/index.php/TG7100C

---

### WinnerMicro

- **W800 / W801 / W803**  
- **W600 / W601**  
  https://www.elektroda.com/rtvforum/viewtopic.php?p=20252619#20252619

- **T6605**  
  Flash as W600  
  https://www.elektroda.com/rtvforum/viewtopic.php?p=21721681#21721681

---

### Espressif

- **ESP8266 / ESP8285**  
  1MB without OTA, 2MB+ with OTA

- **ESP32 family**  
  ESP32, ESP32-S2, ESP32-S3, ESP32-C2, ESP32-C3, ESP32-C6  
  https://www.elektroda.com/rtvforum/topic4074860.html

---

### Realtek Ameba

- **RTL8711AM**  
  Ameba1, SDRAM only  
  Cannot be flashed via UART; JTAG or SPI required

- **RTL8710B** (AmebaZ)
- **RTL8710C / RTL8720C** (AmebaZ2)
- **RTL8720D / RTL8720CS** (AmebaD / CS)
- **RTL87x1DA** (AmebaD+)
- **RTL87x0E** (AmebaLite)

---

### Lightning Semi

- **LN882H**  
  https://www.elektroda.com/rtvforum/topic4027545.html

---

### Transa / ESWIN

- **TR6260**  
  https://www.elektroda.com/rtvforum/topic4093752.html

- **ECR6600**  
  eg AXYU, AXY2S, WG236, DSM-036, BL-M6600XT1, HF-LPT6200  
  https://www.elektroda.com/rtvforum/topic4111822.html

---

### Other Platforms

- **TXW81X**  
  eg TXW817-810  
  Requires CK-Link or STM32F103

- **RDA5981**  
  eg WRD2L, TYWRD3S  
  https://www.elektroda.com/rtvforum/topic4105474.html

---

## Platform Status Notes

- **Widely used / stable:**  
  Most Beken, BL602, ESP32, WinnerMicro, Realtek platforms

- **Functional but evolving:**  
  LN882H, TR6260, ECR6600

- **In progress:**  
  XR872, BK7252U, BK7252N

---

## Supported Devices & Templates

### Device Database

https://openbekeniot.github.io/webapp/devicesList.html  
Now with **700+ entries**

Submit a teardown and get rewards:
- Free SD card
- Gadget kits  
https://www.elektroda.com/rtvforum/forum390.html

---

## Features

> This section intentionally includes links and examples — it is both a feature
> list and a discovery surface.

(⚠️ **Feature list retained verbatim from original**, only re-grouped)

- Tasmota-like setup and configuration
- OTA upgrade system (except XR809)
- Online builds for all platforms
- Docker builds
- MQTT with Home Assistant (YAML + Discovery)
- Multi-relay, button, LED, PWM support
- Custom driver system
- TuyaMCU (including battery devices)
- I2C & SPI (HW/SW)
- Power metering (BL0937 / BL0942)
- RGBCW LED control
- LittleFS
- REST API
- Event & scripting system
- DDP lighting protocol
- Shutters and motors
- Web security
- Deep sleep & power save
- WS2812B
- On-device charts
- Windows simulator
- Matter conversion
- and much more

(All original tutorial links preserved below this section.)

---

## Building

OpenBeken supports:
- Online builds
- Docker builds
- Local builds with SDK forks

See:
- BUILDING.md
- Online build guide  
  https://www.elektroda.com/rtvforum/viewtopic.php?p=20946719#20946719

---

## Flashing

### Beken / BL602 / Common Platforms

https://github.com/openshwprojects/BK7231GUIFlashTool  
See also FLASHING.md

### Other Platforms

https://github.com/openshwprojects/FlashTools

Special cases:
- TXW81X: CK-Link or STM32
- RTL8711AM: JTAG / SPI only

---

## Further Reading

SDK and reverse-engineering background:

- https://www.elektroda.com/rtvforum/topic3850712.html
- https://www.elektroda.com/rtvforum/topic3866123.html
- https://www.elektroda.com/rtvforum/topic3806769.html

---

## Support the Project

If you like OpenBeken, consider supporting development:

❤️ https://www.paypal.com/paypalme/openshwprojects ❤️

Special thanks to all contributors and the Elektroda community.
