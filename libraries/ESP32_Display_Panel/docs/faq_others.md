# Other FAQs

## Can't find drivers for the same model of LCD/Touch device?

For **LCD**, devices with the same interface type (SPI, QSPI, etc.) have similar or even identical driving methods. For example, ILI9341 and GC9A01 have almost identical driver code when using the SPI interface. Therefore, you can try using drivers for other devices with the same interface type.

For **Touch**, drivers for some devices in the same series are compatible, such as CST816S, CST816D, and CST820 having compatible driver code. Therefore, you can check the technical manuals of devices in the same series to determine if there is compatibility (devices with the same I2C address are usually compatible).

If the above methods cannot solve your problem, you can create a [Github Issue](https://github.com/esp-arduino-libs/ESP32_Display_Panel/issues) to request adding a driver.
