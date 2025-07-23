
| Platform                                                | Family          | Wi-Fi  | WPA3 | OTA    | GPIO | GPIO IRQ | UART | PWM  | ADC | Deep sleep | WDT |
|---------------------------------------------------------|-----------------|--------|------|--------|------|----------|------|------|-----|------------|-----|
| BK7231T                                                 | Beken           | ✅     | ✅¹² | ✅    | ✅   | ✅       | ✅   | ✅  | ✅  | ✅        | ✅  |
| BK7231N                                                 | Beken           | ✅     | ✅¹² | ✅    | ✅   | ✅       | ✅   | ✅  | ✅  | ✅        | ✅  |
| BK7231S/BK7231U                                         | Beken           | ✅     | ✅   | ✅¹   | ✅   | ✅       | ✅   | ✅  | ✅  | ✅        | ✅  |
| BK7238                                                  | Beken           | ✅     | ✅   | ✅    | ✅   | ✅       | ✅   | ✅  | ✅  | ✅        | ✅  |
| BK7252                                                  | Beken           | ✅     | ✅   | ✅¹   | ✅   | ✅       | ✅   | ✅  | ✅  | ✅        | ✅  |
| BK7252N                                                 | Beken           | ✅     | ✅   | ✅    | ✅   | ✅       | ✅   | ✅  | ✅  | ✅        | ✅  |
| XR809                                                   | XRadio          | ✅     | ❌   | ❌⁵   | ✅   | ✅       | ✅   | ✅⁸ | ✅  | ✅        | ✅  |
| XR806                                                   | XRadio          | ✅     | ❓   | ✅    | ✅   | ✅       | ✅   | ✅⁸ | ✅  | ✅        | ✅  |
| XR872/XF16                                              | XRadio          | ✅     | ❓   | ✅²   | ✅   | ✅       | ✅   | ✅⁸ | ✅  | ✅        | ✅  |
| BL602/LF686                                             | Bouffalo Lab    | ✅     | ❓   | ✅⁴   | ✅   | ✅       | ✅   | ✅  | ❌  | ❌        | ✅  |
| W800/W801                                               | WinnerMicro     | ✅     | ❌   | ✅    | ✅   | ❌       | ✅   | ✅  | ✅  | ❌        | ✅  |
| W600/W601                                               | WinnerMicro     | ✅     | ❌   | ✅    | ✅   | ✅       | ❓   | ✅  | ✅  | ❌        | ✅  |
| LN882H                                                  | Lightning Semi  | ✅     | ❌   | ✅⁴   | ✅   | ✅       | ❌   | ✅  | ⚠️  | ❌        | ✅  |
| ESP8285/ESP8266                                         | Espressif       | ✅     | ❓   | ✅²'⁴ | ✅   | ✅       | ✅   | ✅⁷ | ❌  | ⚠️        | ❓⁹ |
| ESP32<br>-C2<br>-C3<br>-C5<br>-C6<br>-C61<br>-S2<br>-S3 | Espressif       | ✅     | ❓   | ✅⁴   | ✅   | ✅       | ✅   | ✅  | ❓  | ✅¹⁰      | ✅  |
| TR6260                                                  | Transa Semi     | ✅     | ❌   | ⚠️³'⁴ | ✅   | ❌       | ❌   | ✅⁸ | ❌  | ❌        | ✅⁹ |
| RTL8711AM (Ameba1)                                      | Realtek         | ✅     | ❓   | ✅⁴   | ✅   | ✅       | ✅   | ✅⁸ | ❌  | ❌        | ✅  |
| RTL8710B (AmebaZ)                                       | Realtek         | ✅     | ❓   | ✅⁴   | ✅   | ✅       | ✅   | ✅⁸ | ❌  | ❌        | ✅  |
| RTL8710C/RTL8720C (AmebaZ2)                             | Realtek         | ✅     | ❓   | ✅⁴   | ✅   | ✅       | ✅   | ✅⁸ | ❌  | ❌        | ✅  |
| RTL8720D (AmebaD)<br>RTL872xCSM/RTL8720CS (AmebaCS)     | Realtek         | ✅     | ❓   | ✅⁴   | ✅   | ✅       | ✅   | ✅⁸ | ❌  | ❌        | ✅  |
| RTL8721DA/RTL8711DAF (AmebaDplus)                       | Realtek         | ✅     | ❓   | ✅⁶   | ✅   | ✅       | ✅   | ✅  | ❌  | ❌        | ✅  |
| RTL8720E/RTL8710ECF (AmebaLite)                         | Realtek         | ✅     | ❓   | ✅⁶   | ✅   | ✅       | ✅   | ✅  | ❌  | ❌        | ✅  |
| ECR6600                                                 | ESWIN           | ✅     | ❓   | ✅    | ✅   | ✅       | ✅   | ✅⁸ | ⚠️  | ⚠️¹¹      | ✅  |

✅ - Works
❓ - Not tested
❌ - Not implemented
⚠️ - Broken

¹ Success dependant on partition layout set in bootloader. SPI flash QIO firmware for guaranteed OTA success<br>
² Excluding 1MB variation<br>
³ Implemented, but no tool to generate the file<br>
⁴ No HTTP OTA, only in Web App<br>
⁵ OTA attempt leads to device crash<br>
⁶ Web App OTA may be unstable, HTTP OTA is preferable<br>
⁷ Software PWM, expect flickering<br>
⁸ Be careful with pin assignments, some PWM channels overlap<br>
⁹ WDT is configured in SDK<br>
¹⁰ Timer sleep only, no GPIO wakeup<br>
¹¹ After waking up device will refuse to connect to WiFi until power cycled<br>
¹² Only in _ALT builds<br>
