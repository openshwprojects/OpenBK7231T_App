
#include "lwip/sockets.h"
#include "lwip/ip_addr.h"
#include "lwip/inet.h"
#include "lwip/netdb.h"
#if PLATFORM_XR809
#include "my_lwip2_mqtt_replacement.h"
#else
#include "lwip/apps/mqtt.h"
#endif

extern ip_addr_t mqtt_ip;
extern mqtt_client_t* mqtt_client;

void mqtt_example_init(void);
void example_do_connect(mqtt_client_t *client);
void example_publish(mqtt_client_t *client, int channel, int iVal);
void MQTT_RunEverySecondUpdate();



