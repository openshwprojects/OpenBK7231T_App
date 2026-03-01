/*
  Berry bindings for the built-in MQTT server.

  Berry usage:
    # Subscribe to all messages:
    ms_subscribe("#", def (topic, payload)
      print("MQTTS: " + topic + " = " + payload)
    end)

    # Subscribe with wildcard:
    ms_subscribe("cmnd/+/POWER", def (topic, payload)
      print("POWER: " + topic + " = " + payload)
    end)

    # Subscribe to a specific topic:
    ms_subscribe("stat/mydevice/RESULT", def (topic, payload)
      print("result: " + payload)
    end)


// ============================================
// NOTE: you don't need to manually call init.
// Sample module (save as autoexec.be):

autoexec = module('autoexec')

autoexec.init = def()
        autoexec.power_sub = ms_subscribe("cmnd/+/POWER", def (topic, payload)
                print("Berry POWER: " + topic + " = " + payload)
        end)
        autoexec.get_sub = ms_subscribe("+/+/get", def (topic, payload)
                print("Berry GET: " + topic + " = " + payload)
        end)
         autoexec.ip_sub = ms_subscribe("+/ip", def (topic, payload)
                 print("Berry Got IP Report: " + topic + " = " + payload)
		end) 
		autoexec.uptime_sub = ms_subscribe("+/uptime", def (topic, payload)
			print("Berry Got UpTime Report: " + topic + " = " + payload)
		end)
        autoexec.ha_sub = ms_subscribe("homeassistant/+", def (topic,payload) 
			print("Berry HA Discovery: " + topic + " = " + payload)
		end)
        autoexec.con_sub = ms_subscribe("+/connected", def (topic, payload)
			print("Berry Connected: " + topic + " = " + payload)
		end)
        autoexec.sensor_sub = ms_subscribe("stat/+/SENSOR", def (topic, payload)
                print("Berry SENSOR: " + topic + " = " + payload)
        end)
        autoexec.stat_sub = ms_subscribe("stat/+/+", def (topic,payload)
			print("Berry STAT: " + topic + " = " + payload)
		end)
end

return autoexec


// ============================================
// NOTE: you don't need to manually call init.
// Smaller sample:


autoexec = module('autoexec')

autoexec.init = def()
                autoexec.get_sub = ms_subscribe("+/+/get", def (topic, payload)
                                print("Berry GET: " + topic + " = " + payload)
                end)
end

return autoexec


// ============================================
// NOTE: you don't need to manually call init.
// Tasmota power sample:


autoexec = module('autoexec')

autoexec.init = def()
        autoexec.power_state_sub = ms_subscribe("stat/+/POWER", def (topic,payload)
			print("Berry POWER STATE: " + topic + " = " + payload)
		end)
        autoexec.power_state_sub = ms_subscribe("stat/+/POWER1", def (topic,payload) 
			print("Berry POWER1 STATE: " + topic + " = " + payload)
		end)
        autoexec.power_state_sub = ms_subscribe("stat/+/POWER2", def (topic,payload)
			print("Berry POWER2 STATE: " + topic + " = " + payload)
		end)
	end

return autoexec


// ============================================
// NOTE: you don't need to manually call init.
// Tasmota power sample:



autoexec = module('autoexec')

autoexec.init = def()
        autoexec.power_state_sub = ms_subscribe("stat/tasmota_476739/POWER2", def (topic, payload) 
				ms_publish("cmnd/obk174083A4/POWER",payload);
                print("Berry POWER STATE: " + topic + " = " + payload)
        end)
end

return autoexec


*/

#include "../new_common.h"
#include "../obk_config.h"

#if ENABLE_DRIVER_MQTTSERVER && ENABLE_OBK_BERRY

#include "../berry/be_run.h"
#include "../logging/logging.h"

extern bvm *g_vm;

// Subscription entry: topic filter + Berry closure ID
typedef struct berrySub_s {
  char *topicFilter;
  int closureId;
  struct berrySub_s *next;
} berrySub_t;

static berrySub_t *g_berrySubs = NULL;

// Reuse MQTTS_TopicMatch from drv_mqttServer.c
extern int MQTTS_TopicMatch(const char *topic, const char *filter);

// Called from drv_mqttServer.c on every incoming PUBLISH
int MQTTS_Berry_OnPublish(const char *topic, const byte *payload,
                          int payloadLen) {
  if (!g_vm || !g_berrySubs)
    return 0;

  // null-terminate payload for Berry string
  char payloadStr[256];
  int pLen = payloadLen < 255 ? payloadLen : 255;
  memcpy(payloadStr, payload, pLen);
  payloadStr[pLen] = 0;

  berrySub_t *s;
  int ran = 0;
  for (s = g_berrySubs; s; s = s->next) {
    if (MQTTS_TopicMatch(topic, s->topicFilter)) {
      berryRunClosureStr(g_vm, s->closureId, topic, payloadStr);
      ran++;
    }
  }
  return ran;
}

// Berry native: ms_subscribe(topic_filter, closure)
// Returns subscription ID (closure ID) on success, nil on failure
static int be_ms_subscribe(bvm *vm) {
  int top = be_top(vm);
  if (top < 2 || !be_isstring(vm, 1) || !be_isfunction(vm, 2)) {
    be_return_nil(vm);
  }

  const char *filter = be_tostring(vm, 1);

  // Suspend the closure to get a closure ID
  if (!be_getglobal(vm, "suspend_closure")) {
    be_return_nil(vm);
  }
  be_pushvalue(vm, 2); // push the closure
  be_call(vm, 1);      // call suspend_closure(closure)

  if (!be_isint(vm, -2)) {
    be_pop(vm, 2);
    be_return_nil(vm);
  }

  int closureId = be_toint(vm, -2);
  be_pop(vm, 2);

  // Create subscription entry
  berrySub_t *sub = (berrySub_t *)malloc(sizeof(berrySub_t));
  if (!sub) {
    be_return_nil(vm);
  }
  sub->topicFilter = strdup(filter);
  sub->closureId = closureId;
  sub->next = g_berrySubs;
  g_berrySubs = sub;

  addLogAdv(LOG_INFO, LOG_FEATURE_GENERAL, "MQTTS Berry: subscribed to '%s'",
            filter);

  be_pushint(vm, closureId);
  be_return(vm);
}

// Berry native: ms_unsubscribe(subscription_id)
static int be_ms_unsubscribe(bvm *vm) {
  if (be_top(vm) < 1 || !be_isint(vm, 1)) {
    be_return_nil(vm);
  }
  int id = be_toint(vm, 1);
  berrySub_t **pp = &g_berrySubs;
  while (*pp) {
    if ((*pp)->closureId == id) {
      berrySub_t *victim = *pp;
      *pp = victim->next;
      addLogAdv(LOG_INFO, LOG_FEATURE_GENERAL, "MQTTS Berry: unsubscribed '%s'",
                victim->topicFilter);
      free(victim->topicFilter);
      free(victim);
      be_pushbool(vm, btrue);
      be_return(vm);
    }
    pp = &(*pp)->next;
  }
  be_pushbool(vm, bfalse);
  be_return(vm);
}

// Forward-declare from drv_mqttServer.c (non-static for Berry access)
extern void MQTTS_ForwardPublish(const byte *topicData, int topicLen,
                                 const byte *payload, int payloadLen,
                                 void *sender);

// Berry native: ms_publish(topic, payload)
static int be_ms_publish(bvm *vm) {
  int top = be_top(vm);
  if (top < 2 || !be_isstring(vm, 1) || !be_isstring(vm, 2)) {
    be_return_nil(vm);
  }
  const char *topic = be_tostring(vm, 1);
  const char *payload = be_tostring(vm, 2);
  int topicLen = strlen(topic);
  int payloadLen = strlen(payload);
  MQTTS_ForwardPublish((const byte *)topic, topicLen, (const byte *)payload,
                       payloadLen, NULL);
  be_pushbool(vm, btrue);
  be_return(vm);
}

// Register Berry native functions
void MQTTS_Berry_Init() {
  if (!g_vm)
    return;
  be_regfunc(g_vm, "ms_subscribe", be_ms_subscribe);
  be_regfunc(g_vm, "ms_unsubscribe", be_ms_unsubscribe);
  be_regfunc(g_vm, "ms_publish", be_ms_publish);
  addLogAdv(LOG_INFO, LOG_FEATURE_GENERAL, "MQTTS Berry: bindings registered");
}

// Free all Berry subscriptions
void MQTTS_Berry_Shutdown() {
  berrySub_t *s = g_berrySubs;
  while (s) {
    berrySub_t *next = s->next;
    free(s->topicFilter);
    free(s);
    s = next;
  }
  g_berrySubs = NULL;
}

#else

// Stubs when Berry or MQTT server is disabled
int MQTTS_Berry_OnPublish(const char *topic, const byte *payload,
                          int payloadLen) {
  return 0;
}
void MQTTS_Berry_Init() {}
void MQTTS_Berry_Shutdown() {}

#endif // ENABLE_DRIVER_MQTTSERVER && ENABLE_OBK_BERRY