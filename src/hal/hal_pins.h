
#ifndef __HAL_OBK_PINS_H__
#define __HAL_OBK_PINS_H__

void HAL_PIN_SetOutputValue(int index, int iVal);
int HAL_PIN_ReadDigitalInput(int index);
void HAL_PIN_Setup_Input_Pulldown(int index);
void HAL_PIN_Setup_Input_Pullup(int index);
void HAL_PIN_Setup_Input(int index);
void HAL_PIN_Setup_Output(int index);
void HAL_PIN_PWM_Stop(int index);
void HAL_PIN_PWM_Start(int index, int freq);
// Value range is 0 to 100, value is clamped
void HAL_PIN_PWM_Update(int index, float value);
int HAL_PIN_CanThisPinBePWM(int index);
const char* HAL_PIN_GetPinNameAlias(int index);
// Translate name like RB5 for OBK pin index
int HAL_PIN_Find(const char *name);

typedef void(*OBKInterruptHandler)(int gpio);
typedef enum {
	INTERRUPT_STUB,
	INTERRUPT_RISING,
	INTERRUPT_FALLING,
	INTERRUPT_CHANGE,

} OBKInterruptType;
void HAL_AttachInterrupt(int pinIndex, OBKInterruptType mode, OBKInterruptHandler function);
void HAL_DetachInterrupt(int pinIndex);

/// @brief Get the actual GPIO pin for the pin index.
/// @param index 
/// @return 
unsigned int HAL_GetGPIOPin(int index);

#endif

