

// i didn't choose the "automatic, per string name" approach because it would be slower
// I think we only need to dedup the built-in MQTT publishes, which can be listed here and defined as constants
typedef enum MQTT_Dedup_Slot_e {
	DEDUP_LED_BASECOLOR_RGB,
	DEDUP_LED_FINALCOLOR_RGB,
	DEDUP_LED_DIMMER,
	DEDUP_LED_TEMPERATURE,
	DEDUP_LED_ENABLEALL,
	// not enabled by default, but a user requested ability also to broadcast full RGBCW format
	DEDUP_LED_FINALCOLOR_RGBCW,
	DEDUP_MAX,
} MQTT_Dedup_Slot_t;

#define DEDUP_EXPIRE_TIME 5

// This will not republish given value if value is the same as in previous publish and if the time passed since last publish is lower than expireTime
OBK_Publish_Result MQTT_PublishMain_StringString_DeDuped(int slotCode, int expireTime, const char* sChannel, const char* valueStr, int flags);
OBK_Publish_Result MQTT_PublishMain_StringInt_DeDuped(int slotCode, int expireTime, const char* sChannel, int val, int flags);
void MQTT_Dedup_Tick();
