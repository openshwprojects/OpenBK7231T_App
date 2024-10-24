#ifndef __PING_H__
#define __PING_H__

#define TLS_CONFIG_WIFI_PING_TEST   (CFG_ON && TLS_CONFIG_HOSTIF)

#if TLS_CONFIG_WIFI_PING_TEST
struct ping_param{
    char host[64];
    u32 interval;/* ms */
    u32 cnt;/* -t */
    u32 src;
};

void ping_test_create_task(void);
void ping_test_start(struct ping_param *para);
void ping_test_stop(void);

int ping_test_sync(struct ping_param *para);
#endif

#endif /* __PING_H__ */

