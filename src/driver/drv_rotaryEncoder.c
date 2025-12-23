#include "../new_common.h"
#include "../new_pins.h"
#include "../quicktick.h"
#include "../cmnds/cmd_public.h"
#include "drv_local.h"
#include "drv_public.h"
#include "../logging/logging.h"
#include "../hal/hal_pins.h"
#include "../mqtt/new_mqtt.h"
#include "drv_rotaryEncoder.h"

// Rotary encoder state machine for quadrature decoding
// Gray code sequence for quadrature encoding: 00 -> 10 -> 11 -> 01 -> 00
static const int g_grayToBinary[] = {0, 1, 3, 2};

// Rotary encoder state tracking
static int g_clkPin = -1;
static int g_dtPin = -1;
static int g_buttonPin = -1;
static int g_clkChannel = -1;
static int g_dtChannel = -1;
static int g_buttonChannel = -1;
static int g_positionChannel = -1;

// State variables
static int g_position = 0;
static int g_lastClkState = 0;
static int g_lastDtState = 0;
static int g_lastButtonState = 0;
static int g_debounceCounter = 0;
static int g_timeSinceLastEvent = 0;

#define DEBOUNCE_TIME_MS 10
#define MIN_TIME_BETWEEN_EVENTS_MS 5

void RotaryEncoder_Init()
{
	addLogAdv(LOG_INFO, LOG_FEATURE_GENERAL, "Initializing Rotary Encoder Driver\r\n");

	// Find first two pins assigned as DigitalInput_n for CLK and DT
	for(int i=0; i<PLATFORM_GPIO_MAX; i++) {
		if(g_cfg.pins.roles[i] == IOR_DigitalInput_n) {
			if(g_clkPin == -1) {
				g_clkPin = i;
			} else if(g_dtPin == -1) {
				g_dtPin = i;
				break;
			}
		}
	}
	// Find optional button pin assigned as Button_n
	g_buttonPin = PIN_FindPinIndexForRole(IOR_Button_n, -1);
	
	// Check if we have at least CLK and DT pins
	if (g_clkPin == -1 || g_dtPin == -1) {
		addLogAdv(LOG_WARN, LOG_FEATURE_GENERAL, "Rotary Encoder: Missing CLK or DT pins\r\n");
		return;
	}
	
	// Get associated channels
	g_clkChannel = g_cfg.pins.channels[g_clkPin];
	g_dtChannel = g_cfg.pins.channels[g_dtPin];
	if (g_buttonPin != -1) {
		g_buttonChannel = g_cfg.pins.channels[g_buttonPin];
	}
	
	// Get optional position tracking channel (second channel for CLK pin)
	g_positionChannel = g_cfg.pins.channels2[g_clkPin];
	
	// Setup pins as inputs with pull-up
	HAL_PIN_Setup_Input_Pullup(g_clkPin);
	HAL_PIN_Setup_Input_Pullup(g_dtPin);
	if (g_buttonPin != -1) {
		HAL_PIN_Setup_Input_Pullup(g_buttonPin);
	}
	
	// Initialize state
	g_lastClkState = HAL_PIN_ReadDigitalInput(g_clkPin);
	g_lastDtState = HAL_PIN_ReadDigitalInput(g_dtPin);
	if (g_buttonPin != -1) {
		g_lastButtonState = HAL_PIN_ReadDigitalInput(g_buttonPin);
	}
	
	addLogAdv(LOG_INFO, LOG_FEATURE_GENERAL, 
		"Rotary Encoder initialized: CLK=%d, DT=%d, Button=%d\r\n", 
		g_clkPin, g_dtPin, g_buttonPin);
}

void RotaryEncoder_RunFrame()
{
	if (g_clkPin == -1 || g_dtPin == -1) {
		return;
	}
	
	g_timeSinceLastEvent += g_deltaTimeMS;
	
	// Read current states
	int clkState = HAL_PIN_ReadDigitalInput(g_clkPin);
	int dtState = HAL_PIN_ReadDigitalInput(g_dtPin);
	
	// Debounce: only process if state has been stable for DEBOUNCE_TIME_MS
	if (clkState != g_lastClkState || dtState != g_lastDtState) {
		g_debounceCounter += g_deltaTimeMS;
		
		if (g_debounceCounter >= DEBOUNCE_TIME_MS) {
			// State has been stable long enough, update
			if (clkState != g_lastClkState || dtState != g_lastDtState) {
				// Quadrature decoding: detect rotation direction
				// Full cycle: 00 -> 10 -> 11 -> 01 -> 00 (clockwise)
				//            00 -> 01 -> 11 -> 10 -> 00 (counter-clockwise)
				
				if (clkState != g_lastClkState) {
					// CLK changed - this indicates rotation
					if (clkState == 1 && dtState != g_lastDtState) {
						// Detecting on rising edge of CLK
						if (dtState == g_lastClkState) {
							// CW direction: CLK 0->1, DT follows
							g_position++;
							EventHandlers_FireEvent(CMD_EVENT_CUSTOM_UP, g_position);
							addLogAdv(LOG_DEBUG, LOG_FEATURE_GENERAL, "Rotary: CW, pos=%d\r\n", g_position);
						} else {
							// CCW direction: CLK 0->1, DT opposite
							g_position--;
							EventHandlers_FireEvent(CMD_EVENT_CUSTOM_DOWN, g_position);
							addLogAdv(LOG_DEBUG, LOG_FEATURE_GENERAL, "Rotary: CCW, pos=%d\r\n", g_position);
						}
						
						// Update position channel if configured
						if (g_positionChannel != -1) {
							CHANNEL_Set(g_positionChannel, g_position, 0);
						}
					}
				}
			}
			
			g_lastClkState = clkState;
			g_lastDtState = dtState;
			g_debounceCounter = 0;
			g_timeSinceLastEvent = 0;
		}
	} else {
		// State is stable, reset debounce counter
		g_debounceCounter = 0;
	}
	
	// Handle optional button
	if (g_buttonPin != -1) {
		int buttonState = HAL_PIN_ReadDigitalInput(g_buttonPin);
		
		if (buttonState != g_lastButtonState) {
			g_debounceCounter += g_deltaTimeMS;
			
			if (g_debounceCounter >= DEBOUNCE_TIME_MS) {
				g_lastButtonState = buttonState;
				
				if (buttonState == 0) {
					// Button pressed (active low)
					EventHandlers_FireEvent(CMD_EVENT_PIN_ONPRESS, 1);
					addLogAdv(LOG_DEBUG, LOG_FEATURE_GENERAL, "Rotary Button: Pressed\r\n");
				} else {
					// Button released
					EventHandlers_FireEvent(CMD_EVENT_PIN_ONRELEASE, 0);
					addLogAdv(LOG_DEBUG, LOG_FEATURE_GENERAL, "Rotary Button: Released\r\n");
				}
				
				if (g_buttonChannel != -1) {
					CHANNEL_Set(g_buttonChannel, buttonState == 0 ? 1 : 0, 0);
				}
				
				g_debounceCounter = 0;
			}
		} else {
			g_debounceCounter = 0;
		}
	}
}