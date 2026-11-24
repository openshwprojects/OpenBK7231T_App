# Bluetooth proxy for Home Assistant
OpenBeken can act as an [ESPHome Bluetooth proxy](https://esphome.io/components/bluetooth_proxy.html) for Home Assistant: the ESPHomeAPI driver serves the native ESPHome API on port 6053 and forwards Bluetooth LE advertisements to HA. On **RTL87X0C** `btproxy` builds it also supports **active connections**: HA can connect to BLE devices through the proxy, discover their services and read, write and subscribe to notifications (e.g. to control BLE locks).

**Setup**
1. Flash a `btproxy` build for your chip (BK7238, BK7231N_ALT, LN882H, RTL87X0C; the BT proxy is always on for RTL8720D/RTL8721DA).
2. Set the startup command (Config -> Change Startup Command), for example:
`backlog BTInit; BTSetScanMode 1; startDriver ESPHomeAPI; BTStartScan`
3. In Home Assistant: Settings -> Devices & services -> Add integration -> ESPHome, enter the device IP and port 6053 (no encryption key). The proxy then shows up as a Bluetooth scanner and BLE integrations use it automatically.

**Notes**
- `BTSetScanMode 1` (active scanning) requests scan responses. Some devices only put important data there, e.g. Tuya BLE devices advertise their protocol version only in the scan response, so integrations for them need active scanning.
- Only one API client (Home Assistant) can be connected at a time.
- Active connections (RTL87X0C): up to 2 simultaneous BLE links. HA writes the notification descriptors itself (the proxy announces the REMOTE_CACHING feature).
- `BTProxyLeds [redChannel] [blueChannel]` drives two LED channels as status lights: blue = Home Assistant connected, red blinking = connecting/discovering, red solid = a BLE device is connected.
- Diagnostics: `BTProxyStatus` logs scan/link state, `BTProxyConnect [MAC] [addrType]` / `BTProxyDisconnect` test connections without HA, and `loglevel 7` adds GATT traces.

Example device: [Tuya Bluetooth SigMesh Gateway JM-MESH01-WG](https://openbekeniot.github.io/webapp/devicesList.html) (CR3L / RTL87X0C), used with a Gimdow A1 Pro Max lock via Home Assistant's `gimdow_ble` integration.