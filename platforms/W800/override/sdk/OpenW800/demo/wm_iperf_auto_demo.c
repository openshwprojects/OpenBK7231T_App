#include <string.h>
#include "wm_include.h"
#include "wm_demo.h"
#include "iperf.h"
#include "lwip/inet.h"
#include "wm_cmdp.h"


#if (DEMO_IPERF_AUTO_TEST && TLS_CONFIG_WIFI_PERF_TEST && TLS_IPERF_AUTO_TEST)


u8 iperf_test_info[15] = {0};

u8 iperf_test_rev[10] = {0};
u8 sever_ip[16];
u16 iperfLocalPort = 0;

extern 	tls_os_queue_t *tht_q;
extern struct tht_param *gThtSys;
extern void CreateThroughputTask(void);

void iperf_start(u8 mode, u8 chnl, u8 interval, u32 maxcnt, u16 localport, u32 severIp)
{

    CreateThroughputTask();
	if (gThtSys)
	{
	    if(mode)
	    {
	        gThtSys->role = 's';
	    }
	    else
	    {
	        gThtSys->role = 'c';

	        inet_ntoa_r(severIp, (char *)sever_ip, 16);
	        strcpy(gThtSys->server_hostname, (char *)sever_ip);
	        printf("ipaddr:%s\n", gThtSys->server_hostname);

	        gThtSys->protocol = Pudp;
	        gThtSys->rate = 0;
	        gThtSys->block_size = 0;
	    }

	    gThtSys->localport = localport;

	    gThtSys->report_interval = interval;
	    gThtSys->duration = maxcnt;
	    gThtSys->port = chnl + PORT - 1;

	    tls_os_queue_send(tht_q, (void *)TLS_MSG_WIFI_PERF_TEST_START, 0);
	}
}


int demo_iperf_auto_test(u8 *ssid, u8 csmode, u8 remoteIP, u8 bgnrate, u8 pcrate, u8 interval, u32 maxcnt)
{
    int ret = -1;
    u16 i;
    int ipefTestSock;
    u8 destip[4];
    u32 severIp;
    struct tls_cmd_wl_hw_mode_t hw_mode;
    struct tls_cmd_link_status_t lk;


    memset(&hw_mode, 0, sizeof(struct tls_cmd_wl_hw_mode_t));
    hw_mode.hw_mode = 2;
    hw_mode.max_rate = bgnrate;
    ret = tls_cmd_set_hw_mode(&hw_mode, 1);

    if (!ssid)
    {
        return WM_FAILED;
    }

    printf("\nssid:%s\n", ssid);

    ret = tls_wifi_connect(ssid, strlen((char *)ssid), NULL, 0);
    while(WM_WIFI_JOINED != tls_wifi_get_state())
    {
        tls_os_time_delay(2);
    }

    memset(&lk, 0, sizeof(struct tls_cmd_link_status_t));
    while(lk.ip[0] == 0)
    {
        tls_cmd_get_link_status(&lk);
        tls_os_time_delay(2);
    }

    memcpy(&destip[0], &lk.ip[0], 4);
    destip[3] = remoteIP;
    memcpy(&severIp, &destip[0], 4);

    iperfLocalPort ++;
    if(iperfLocalPort > 20)
    {
        iperfLocalPort = 0;
    }

    ipefTestSock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    if(ipefTestSock < 0)
    {
        printf("iperf test socket creat err\n");
        return WM_FAILED;;
    }

    if(csmode)		//iperf 作为服务端
    {
        printf("iperf test sever start\n");
        iperf_start(csmode, destip[2] - 100, interval, maxcnt, iperfLocalPort, severIp);
    }

    struct sockaddr_in localAddr;
    memset(&localAddr, 0, sizeof(struct sockaddr_in));
    localAddr.sin_family = AF_INET;
    localAddr.sin_port = htons(20000 + iperfLocalPort);
    memcpy(&localAddr.sin_addr.s_addr, &lk.ip[0], 4);
    printf("iperf test port:%d\n", 20000 + iperfLocalPort);

    if (bind(ipefTestSock, (const struct sockaddr *)&localAddr, sizeof(const struct sockaddr_in)) < 0)
    {
        printf("iperf test bind err\n");
        return WM_FAILED;
    }

    struct sockaddr_in iperfAddr;
    memset(&iperfAddr, 0, sizeof(struct sockaddr_in));
    iperfAddr.sin_family = AF_INET;
    iperfAddr.sin_port = htons(30000);
    iperfAddr.sin_addr.s_addr = severIp;

    i = 0;
    while (0 != connect(ipefTestSock, (const struct sockaddr *)&iperfAddr, sizeof(const struct sockaddr_in)))
    {
        printf("iperf test connect err\n");
        if(i++ > 5)
        {
            closesocket(ipefTestSock);
            return WM_FAILED;
        }
    }

    iperf_test_info[0] = csmode;
    iperf_test_info[1] = destip[2] - 100;
    memcpy(&iperf_test_info[2], &lk.ip[0], 4);
    iperf_test_info[6] = pcrate;
    iperf_test_info[7] = interval;
    memcpy(&iperf_test_info[8], &maxcnt, 4);

    if(send(ipefTestSock, (const char *)iperf_test_info, 12, 0) < 0)
    {
        printf("iperf test tcp send err\n");
        closesocket(ipefTestSock);
        return WM_FAILED;
    }

    if(csmode == 0)
    {
        ret = recv(ipefTestSock, iperf_test_rev, 10, 0);
        if(ret > 0)
        {
            ret = strcmp((const char *)iperf_test_rev, "OK");
            closesocket(ipefTestSock);
            if(ret == 0)
            {
                printf("iperf test client start\n");
                tls_os_time_delay(100);
                iperf_start(csmode, destip[2] - 100, interval, maxcnt, iperfLocalPort, severIp);
            }
        }
        else
        {
            printf("iperf test tcp recv err\n");
            closesocket(ipefTestSock);
            return WM_FAILED;
        }
    }
    else
    {
        closesocket(ipefTestSock);
    }

    return WM_SUCCESS;
}



#endif


