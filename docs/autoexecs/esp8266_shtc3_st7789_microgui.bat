SHTC3_SetErrorOutput CH10
startDriver HWSPI
startDriver ST7789 hspi NA IO15 IO2 CH4 135 240 53 40 180
startDriver MicroUI st7789
addEventHandler OnClick 0 OnKeyPress TAB
addEventHandler OnHold 0 OnKeyPress ENTER