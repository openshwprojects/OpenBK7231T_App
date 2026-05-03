# 其他 FAQ

## 找不到相同型号的 LCD/Touch 设备驱动？

对于 **LCD**，具有相同接口类型（SPI、QSPI 等）的设备的驱动方式是类似的，甚至是相同的,例如 ILI9341 和 GC9A01 在使用 SPI 接口时，驱动代码几乎相同。因此,您可以尝试使用其他相同接口类型的设备驱动。

对于 **Touch**，部分相同系列设备的驱动是兼容的，如 CST816S 和 CST816D 以及 CST820 的驱动代码是兼容的。因此，您可以查阅相同系列设备的技术手册判断是否存在兼容性（如具有相同的 I2C 地址的设备通常是兼容的）。

如果上述方法无法解决问题，您可以创建 [Github Issue](https://github.com/esp-arduino-libs/ESP32_Display_Panel/issues) 请求添加驱动。
