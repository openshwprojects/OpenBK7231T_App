
alias mode_wifi setPinRole 10 WifiLED_n
alias mode_relay setPinRole 10 LED_n

// at reboot, set WiFiLEd
mode_wifi
// then, setup handlers
addChangeHandler WiFiState == 4 mode_relay 
addChangeHandler WiFiState != 4 mode_wifi 