#ifdef WINDOWS

#include "selftest_local.h"
#include "../hal/hal_wifi.h"

void Test_MQTT_Get_Relay() {
	SIM_ClearOBK(0);
	SIM_ClearAndPrepareForMQTTTesting("myTestDevice", "bekens");

	PIN_SetPinRoleForPinIndex(24, IOR_Relay);
	PIN_SetPinChannelForPinIndex(24, 1);

	SIM_ClearMQTTHistory();


}
void Test_MQTT_Get_LED_EnableAll() {
	SIM_ClearOBK(0);
	SIM_ClearAndPrepareForMQTTTesting("myTestDevice", "bekens");


	PIN_SetPinRoleForPinIndex(24, IOR_PWM);
	PIN_SetPinChannelForPinIndex(24, 1);

	PIN_SetPinRoleForPinIndex(26, IOR_PWM);
	PIN_SetPinChannelForPinIndex(26, 2);


	SIM_ClearMQTTHistory();
	// changing led_enableAll from cmnd will cause MQTT led_enableAll publish
	for (int i = 0; i < 15; i++) {
		const char *tgState;
		if (i % 2 == 0)
			tgState = "1";
		else
			tgState = "0";
			
#if 1
		// send set
		SIM_SendFakeMQTT("cmnd/myTestDevice/led_enableAll", tgState);
#else
		// cause an error
		SIM_SendFakeMQTT("cmnd/myTestDevice/led_enableAll", "0");
#endif
		// expect get reply - led_enableAll will publish its value on change
		SELFTEST_ASSERT_HAD_MQTT_PUBLISH_STR("myTestDevice/led_enableAll/get", tgState, false);
		SIM_ClearMQTTHistory();

		// wait a bit
		Sim_RunFrames(20, false);
		// sending empty get should give us a state dump
		SIM_ClearMQTTHistory();
		SIM_SendFakeMQTT("myTestDevice/led_enableAll/get", "");
		Sim_RunFrames(4, false);
		// expect get reply - led_enableAll will publish value because we have requested it
		SELFTEST_ASSERT_HAD_MQTT_PUBLISH_STR("myTestDevice/led_enableAll/get", tgState, false);
		SIM_ClearMQTTHistory();

	}

	// let's do the same, but for dimmer, and also check if it works

	// make sure it's on
	SIM_SendFakeMQTT("cmnd/myTestDevice/led_enableAll", "1");

	SIM_ClearMQTTHistory();
	// changing led_dimmer from cmnd will cause MQTT led_dimmer publish
	for (int i = 0; i < 15; i++) {
		char tgState[32];
		int randVal = abs(rand() % 50) + 25;
		sprintf(tgState, "%i", randVal);

#if 1
		// send set
		SIM_SendFakeMQTT("cmnd/myTestDevice/led_dimmer", tgState);
#else
		// cause an error
		SIM_SendFakeMQTT("cmnd/myTestDevice/led_dimmer", "0");
#endif
		// expect get reply - led_dimmer will publish its value on change
		SELFTEST_ASSERT_HAD_MQTT_PUBLISH_STR("myTestDevice/led_dimmer/get", tgState, false);
		SIM_ClearMQTTHistory();

		// wait a bit
		Sim_RunFrames(20, false);
		// sending empty get should give us a state dump
		SIM_ClearMQTTHistory();
		SIM_SendFakeMQTT("myTestDevice/led_dimmer/get", "");
		Sim_RunFrames(4, false);
		// expect get reply - led_dimmer will publish value because we have requested it
		SELFTEST_ASSERT_HAD_MQTT_PUBLISH_STR("myTestDevice/led_dimmer/get", tgState, false);
		SIM_ClearMQTTHistory();

	}
	SIM_ClearMQTTHistory();

	// the same for led_temperature
	// changing led_temperature from cmnd will cause MQTT led_temperature publish
	for (int i = 0; i < 15; i++) {
		char tgState[32];
		int randVal = abs(rand() % 300) + 160;
		sprintf(tgState, "%i", randVal);

#if 1
		// send set
		SIM_SendFakeMQTT("cmnd/myTestDevice/led_temperature", tgState);
#else
		// cause an error
		SIM_SendFakeMQTT("cmnd/myTestDevice/led_temperature", "0");
#endif
		// expect get reply - led_dimmer will publish its value on change
		SELFTEST_ASSERT_HAD_MQTT_PUBLISH_STR("myTestDevice/led_temperature/get", tgState, false);
		SIM_ClearMQTTHistory();

		// wait a bit
		Sim_RunFrames(20, false);
		// sending empty get should give us a state dump
		SIM_ClearMQTTHistory();
		SIM_SendFakeMQTT("myTestDevice/led_temperature/get", "");
		Sim_RunFrames(4, false);
		// expect get reply - led_dimmer will publish value because we have requested it
		SELFTEST_ASSERT_HAD_MQTT_PUBLISH_STR("myTestDevice/led_temperature/get", tgState, false);
		SIM_ClearMQTTHistory();

	}

	CFG_SetFlag(OBK_FLAG_MQTT_NEVERAPPENDGET, true);
	for (int i = 0; i < 15; i++) {
		char tgState[32];
		int randVal = abs(rand() % 300) + 160;
		sprintf(tgState, "%i", randVal);

#if 1
		// send set
		SIM_SendFakeMQTT("cmnd/myTestDevice/led_temperature", tgState);
#else
		// cause an error
		SIM_SendFakeMQTT("cmnd/myTestDevice/led_temperature", "0");
#endif
		// expect get reply - led_dimmer will publish its value on change - NO GET SUFIX AS DISABLED IN FLAGS
		SELFTEST_ASSERT_HAD_MQTT_PUBLISH_STR("myTestDevice/led_temperature", tgState, false);
		SIM_ClearMQTTHistory();

		// wait a bit
		Sim_RunFrames(20, false);
		// sending empty get should give us a state dump
		SIM_ClearMQTTHistory();
		SIM_SendFakeMQTT("myTestDevice/led_temperature/get", "");
		Sim_RunFrames(4, false);
		// expect get reply - led_dimmer will publish value because we have requested it - NO GET SUFIX AS DISABLED IN FLAGS
		SELFTEST_ASSERT_HAD_MQTT_PUBLISH_STR("myTestDevice/led_temperature", tgState, false);
		SIM_ClearMQTTHistory();
	}
}



#endif
