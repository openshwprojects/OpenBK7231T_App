/* Copyright 2026 kamilsss655
 * https://github.com/kamilsss655
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 *     Unless required by applicable law or agreed to in writing, software
 *     distributed under the License is distributed on an "AS IS" BASIS,
 *     WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *     See the License for the specific language governing permissions and
 *     limitations under the License.
 */

#include "safety.h"
#include "../cmnds/cmd_public.h"
#include "../logging/logging.h"
#include "../new_common.h"
#if ENABLE_SAFETY

#define LED_THROTTLE_PERIOD 15        	      // seconds between each throttling step (1-1000s)

											  // idea: these can probably be made configurable by user in the future:
static const float MAX_TEMP = 95.0; 		  // any temperature higher than this will enable thermal throttling (60.0-130.0C)
static const int LED_THROTTLE_STEP_SMALL = 1; // percentage throttle small step for the LED dimmer (1-100%)
static const int LED_THROTTLE_STEP_BIG = 5;   // percentage throttle big step for the LED dimmer (1-100%)

static int ledThrottleCountdownSeconds = LED_THROTTLE_PERIOD; // countdown to next led throttle step

void SAFETY_OnEverySecond()
{
	#if ENABLE_LED_BASIC
	led_safety_tick_1s();
	// other device types can be defined here
	#endif
}

// led related safety invoked every second
static void led_safety_tick_1s(void)
{
	// if led is overheated and on
	if(g_wifi_temperature > MAX_TEMP && LED_GetEnableAll())
	{
		if(ledThrottleCountdownSeconds > 0) // if it is too soon to throttle
		{
			ledThrottleCountdownSeconds--; // countdown until next throttle step down
		}
		else
		{	
			led_throttle_brightness(); // throttle the led
			ledThrottleCountdownSeconds = LED_THROTTLE_PERIOD; // start the countdown again
		}
	}
}

// throttle led brightness based on current conditions
static void led_throttle_brightness(void)
{
	int brightness = LED_GetDimmer(); // get current brightness 0-100
	int new_brightness = 0; // target brightness
	float temp_overshoot = (g_wifi_temperature > MAX_TEMP) ? (g_wifi_temperature - MAX_TEMP) : 0.0f; // calculate temperature overshoot

	if(temp_overshoot > 0.0f && temp_overshoot <= 5.0) {
		new_brightness = (brightness > LED_THROTTLE_STEP_SMALL) ? (brightness - LED_THROTTLE_STEP_SMALL) : 0; // if overshoot is small - use small steps
	}
	else if (temp_overshoot > 5.0 && temp_overshoot <= 20.0) {
		new_brightness = (brightness > LED_THROTTLE_STEP_BIG) ? (brightness - LED_THROTTLE_STEP_BIG) : 0; // if overshoot is big - use big steps
	}
    else if (temp_overshoot > 20.0)
	{
		new_brightness = 0; // if overshoot is huge set brightness to 0 - emergency shutdown
	}

	LED_SetDimmer(new_brightness);
	addLogAdv (LOG_WARN, LOG_FEATURE_EVENT, "Temp reached: %.2f. Max temp: %.2f. Throttling dimmer to: %i", g_wifi_temperature, MAX_TEMP, new_brightness);
}
#endif