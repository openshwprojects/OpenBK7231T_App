
	led_gamma_control () - led_gammaCtrl command handler, usage: led_gammaCtrl <sub-command> [parameter(s)]
		sub-commands:
			cal [f f f]   - set RGB calibration values from color-picker or parameters
			gamma <f>     - LED gamma correction, range 1.0-3.0: 1.0 = gamma off, default = 2.2
			brtMinRGB <f> - minimum brightness in % for RGB mode, range 0.0-10.0%, default = 0.1
			brtMinCW <f>  - minimum brightness in % for CW mode, range 0.0-10.0%, default = 0.1
			list          - list settings (and enable additional messages)
		Results will be displayed as Logs Info:CFG: messages (may want to turn off MAIN, MQTT and GEN to silence other messages)
		Any sub-command will enable channel messages containing output values in %

		Calibration sequence:
			1: clear current calibration values (= 1.0, default) by one of these methods:
			   a: set color-picker to "white", i.e. #FFFFFF and enter command "led_gammaCtrl cal"
			   b: or set values directly with command "led_gammaCtrl cal 1.0 1.0 1.0"
			2: adjust RGB LEDs to emit white light of preferred temperature and brightness
			3: enter command "led_gammaCtrl cal" - if successful the settings will be listed and stored in flash

		Use command "gamma <f>" to set gamma value. Range 1.0-3.0: 1.0 = gamma off, default is 2.2.
		  To fully disable gamma correction you may also want to set brtMinX to 0.0.
		Use commands "brtMinRGB <f>" and "brtMinCW <f>" to set minimum brightness for RGB and CW LEDs.
			Valid range is 0.0-10.0% of full range (including brightness 0). "channel messages" is useful in finding suitable
			range: adjust brightness to desired minimum level and use the highest channel % value as brtMinX parameter.
			Channels 0-2 is RGB, channels 3-4 is CW
		Use command "list" to view gamma and calibration settings.