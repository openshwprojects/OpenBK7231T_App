
extern "C" {
	// pins control simulation
	void SIM_SetSimulatedPinValue(int pinIndex, bool bHigh);
	bool SIM_GetSimulatedPinValue(int pinIndex);
	bool SIM_IsPinInput(int index);
	bool SIM_IsPinPWM(int index);
	int SIM_GetPWMValue(int index);
	// flash control simulation
	void SIM_SetupFlashFileReading(const char *flashPath);
	void SIM_SaveFlashData(const char *flashPath);
	void SIM_SetupNewFlashFile(const char *flashPath);
	void SIM_SetupEmptyFlashModeNoFile();
	void SIM_DoFreshOBKBoot();
	void SIM_ClearOBK();
};
