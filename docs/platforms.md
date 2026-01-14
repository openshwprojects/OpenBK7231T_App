| Platform                                                | Family         | GPIO | WPA3 | OTA     | GPIO IRQ | UART | PWM  | ADC | Deep sleep | WDT | SPI LED | IR |
|---------------------------------------------------------|----------------|------|------|---------|----------|------|------|-----|------------|-----|---------|----|
| BK7231T                                                 | Beken          | ✅   | ✅¹² | ✅     | ✅      | ✅   | ✅  | ✅  | ✅         | ✅  | ✅¹²   | ✅ |
| BK7231N                                                 | Beken          | ✅   | ✅¹² | ✅     | ✅      | ✅   | ✅  | ✅  | ✅         | ✅  | ✅     | ✅ |
| BK7231S<br>BK7231U                                      | Beken          | ✅   | ✅   | ✅¹    | ✅      | ✅   | ✅  | ✅  | ✅         | ✅  | ✅     | ✅ |
| BK7238                                                  | Beken          | ✅   | ✅   | ✅     | ✅      | ✅   | ✅  | ✅  | ✅         | ✅  | ✅     | ✅ |
| BK7252                                                  | Beken          | ✅   | ✅   | ⚠️¹'¹⁴ | ✅      | ✅   | ✅  | ✅  | ✅         | ✅  | ✅     | ✅ |
| BK7252N                                                 | Beken          | ✅   | ✅   | ✅     | ✅      | ✅   | ✅  | ✅  | ✅         | ✅  | ✅     | ✅ |
| XR809                                                   | XRadio         | ✅   | ❌   | ❌⁵    | ✅      | ✅   | ✅⁸ | ✅  | ✅         | ✅  | ❌     | ❌ |
| XR806                                                   | XRadio         | ✅   | ✅   | ✅     | ✅      | ✅   | ✅⁸ | ✅  | ✅         | ✅  | ❌     | ❌ |
| XR872/XF16                                              | XRadio         | ✅   | ✅   | ✅²    | ✅      | ✅   | ✅⁸ | ✅  | ✅         | ✅  | ❌     | ❌ |
| BL602/LF686                                             | Bouffalo Lab   | ✅   | ✅   | ✅⁴    | ✅      | ✅   | ✅  | ❌  | ✅         | ✅  | ✅     | ✅ |
| W800/W801                                               | Winner Micro   | ✅   | ❌   | ✅     | ✅      | ✅   | ✅  | ✅  | ❌         | ✅  | ❌     | ❌ |
| W600/W601                                               | Winner Micro   | ✅   | ❌   | ✅     | ✅      | ✅   | ✅  | ✅  | ❌         | ✅  | ❌     | ❌ |
| LN882H                                                  | Lightning Semi | ✅   | ✅   | ✅⁴    | ✅      | ❌   | ✅  | ❗️  | ❌         | ✅  | ✅     | ✅ |
| ESP8266<br>ESP8285                                      | Espressif      | ✅   | ⚠️¹³ | ✅²'⁴  | ✅      | ✅   | ✅⁷ | ❌  | ❗️         | ❓⁹ | ❌     | ❌ |
| ESP32<br>-C2<br>-C3<br>-C5<br>-C6<br>-C61<br>-S2<br>-S3 | Espressif      | ✅   | ⚠️¹³ | ✅⁴    | ✅      | ✅   | ✅  | ❓  | ✅¹⁰       | ✅  | ✅     | ❌ |
| TR6260                                                  | Transa Semi    | ✅   | ❌   | ❗️³'⁴  | ✅      | ❌   | ✅⁸ | ❌  | ❌         | ✅⁹ | ❌     | ❌ |
| RTL8711AM (Ameba1)                                      | Realtek        | ✅   | ❗️   | ✅⁴    | ✅      | ✅   | ✅⁸ | ❌  | ❌         | ✅  | ✅     | ❌ |
| RTL8710B (AmebaZ)                                       | Realtek        | ✅   | ✅   | ✅⁴    | ✅      | ✅   | ✅⁸ | ❌  | ❌         | ✅  | ✅     | ❌ |
| RTL8710C<br>RTL8720C (AmebaZ2)                          | Realtek        | ✅   | ✅   | ✅⁴    | ✅      | ✅   | ✅⁸ | ➖  | ❌         | ✅  | ✅     | ✅ |
| RTL8720D (AmebaD)<br>RTL872xCSM<br>RTL8720CS (AmebaCS)  | Realtek        | ✅   | ✅   | ✅⁴    | ✅      | ✅   | ✅⁸ | ❌  | ❌         | ✅  | ✅     | ❗️ |
| RTL8721DA<br>RTL8711DAF (AmebaDplus)                    | Realtek        | ✅   | ✅   | ✅     | ✅      | ✅   | ✅  | ❌  | ❌         | ✅  | ✅     | ❗️ |
| RTL8720E<br>RTL8710ECF (AmebaLite)                      | Realtek        | ✅   | ✅   | ✅     | ✅      | ✅   | ✅  | ❌  | ❌         | ✅  | ✅     | ❗️ |
| ECR6600                                                 | ESWIN          | ✅   | ✅   | ✅     | ✅      | ✅   | ✅⁸ | ❗️  | ❗️¹¹       | ✅  | ❌     | ❌ |
| TXW81X                                                  | Taixin         | ✅   | ❌   | ❗️     | ❓      | ❌   | ❌  | ❌  | ❌         | ❓  | ❌     | ❌ |
| RDA5981                                                 | RDA            | ✅   | ❌   | ✅     | ✅      | ✅   | ✅  | ❌  | ❌         | ✅  | ➖     | ❌ |


✅ - Works<br>
❓ - Not tested<br>
❌ - Not implemented<br>
❗️ - Broken<br>
⚠️ - Warning<br>
➖ - Not applicable<br>

¹ Success dependant on partition layout set in bootloader. SPI flash QIO firmware for guaranteed OTA success<br>
² Excluding 1MB variation<br>
³ Implemented, but no tool to generate the file<br>
⁴ No HTTP OTA, only in Web App<br>
⁵ OTA attempt leads to device crash<br>
⁶ Web App OTA is broken, use HTTP OTA<br>
⁷ Software PWM, expect flickering<br>
⁸ Be careful with pin assignments, some PWM channels overlap<br>
⁹ WDT is configured in SDK<br>
¹⁰ Timer sleep only, no GPIO wakeup<br>
¹¹ After waking up device will refuse to connect to WiFi until power cycled<br>
¹² Only in _ALT builds<br>
¹³ Must be manually enabled (CONFIG_ESP8266_WIFI_ENABLE_WPA3_SAE/CONFIG_ESP_WIFI_ENABLE_WPA3_SAE to y in sdkconfig.defaults)<br>
¹⁴ OTA on Tuya BK7252 is not supported (stock bootloader won't do anything, custom won't encrypt main partition on unpack, bricking it)<br>
