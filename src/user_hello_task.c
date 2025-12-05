#include "new_common.h"
#include "new_pins.h"
#include "new_cfg.h"
#include "new_http.h"
#include "new_mqtt.h"
#include "http_client.h"
#include "lwip/sockets.h"
#include "cmsis_os.h"
#include "../logging/logging.h"

#define HELLO_SEND_INTERVAL_MS 10000

static osTimerId_t helloTimer;
static const char *HELLO_IP = "192.168.61.201";   // <-- сюда нужный IP
static const int HELLO_PORT = 9090;             // <-- сюда нужный порт

static void HelloTask_Send(void *arg) {
    (void)arg;

    struct sockaddr_in addr;
    int sock;
    const char *msg = "Hello";

    sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (sock < 0) {
        addLogAdv(LOG_ERROR, LOG_FEATURE_MAIN, "HelloTask: socket create failed");
        return;
    }

    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(HELLO_PORT);
    addr.sin_addr.s_addr = inet_addr(HELLO_IP);

    sendto(sock, msg, strlen(msg), 0, (struct sockaddr *)&addr, sizeof(addr));
    closesocket(sock);

    addLogAdv(LOG_INFO, LOG_FEATURE_MAIN, "HelloTask: sent UDP message");
}

// Инициализация задачи
void HelloTask_Init(void) {
    const osTimerAttr_t timer_attr = {
        .name = "helloTimer"
    };

    helloTimer = osTimerNew(HelloTask_Send, osTimerPeriodic, NULL, &timer_attr);
    if (helloTimer != NULL) {
        osTimerStart(helloTimer, HELLO_SEND_INTERVAL_MS);
        addLogAdv(LOG_INFO, LOG_FEATURE_MAIN, "HelloTask started");
    } else {
        addLogAdv(LOG_ERROR, LOG_FEATURE_MAIN, "HelloTask: failed to create timer");
    }
}

