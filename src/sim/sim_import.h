#ifndef __SIM_IMPORT_H__
#define __SIM_IMPORT_H__

#ifdef __cplusplus
extern "C" {
#endif
	// pins control simulation
	void SIM_SetSimulatedPinValue(int pinIndex, bool bHigh);
	bool SIM_GetSimulatedPinValue(int pinIndex);
	bool SIM_IsPinInput(int index);
	bool SIM_IsPinPWM(int index);
	bool SIM_IsPinADC(int index);
	void SIM_SetVoltageOnADCPin(int index, float v);
	void SIM_SetIntegerValueADCPin(int index, int v);
	int SIM_GetPWMValue(int index);
	// flash control simulation
	void SIM_SetupFlashFileReading(const char *flashPath);
	void SIM_SaveFlashData(const char *flashPath);
	void SIM_SetupNewFlashFile(const char *flashPath);
	void SIM_SetupEmptyFlashModeNoFile();
	void SIM_ClearOBK(const char *flashPath);
	bool SIM_IsFlashModified();
	float SIM_GetDeltaTimeSeconds();
#ifdef __cplusplus
}
#endif

#endif // __SIM_IMPORT_H__
