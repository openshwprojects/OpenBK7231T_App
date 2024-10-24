#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>

#include "wm_include.h"
#include "tls_common.h"
#include "lwip/sockets.h"
#include "airkiss.h"
#include "wm_wifi_oneshot.h"

#if TLS_CONFIG_AIRKISS_MODE_ONESHOT

#define AIRKISS_BSSID_CONNECT_ENABLE      1/* 该功能需要使用带log的库 */

/* 局域网发现 */
#define TLS_CONFIG_AIRKISS_LAN            0

/* airkiss debug switcher */
#define AIRKISS_DEBUG                     0

/* aes-128 key */
#if AIRKISS_ENABLE_CRYPT
#define ONESHOT_AIRKISS_AES_KEY           "winnermicro_wifi"
#endif

/* udp广播包数目 */
#define ONESHOT_AIRKISS_REPLY_CNT_MAX     50

/* udp广播端口 */
#define ONESHOT_AIRKISS_REMOTE_PORT      10000

#define ONESHOT_AIRKISS_SSID_LEN_MAX      32
#define ONESHOT_AIRKISS_PWD_LEN_MAX       64

bool is_airkiss = FALSE;

static u8 random4reply = 0;

/* 0-->12: channel 1-->13 */
#define ONESHOT_AIRKISS_CHANNEL_ID_MIN    0
#define ONESHOT_AIRKISS_CHANNEL_ID_MAX    12


static airkiss_context_t pakcontext[1]  = {0};

#if AIRKISS_DEBUG
#define AIRKISS_PRINT printf
#else
#define AIRKISS_PRINT(s, ...)
#endif

#if AIRKISS_BSSID_CONNECT_ENABLE
static u8 ak_bssid[ETH_ALEN] = {0};
#endif

#if TLS_CONFIG_AIRKISS_LAN
void airkiss_lan_task_create(void);
#endif

void oneshot_airkiss_send_reply(void)
{
    u8 idx;
    int socket_num = 0;
    struct sockaddr_in addr;

    if (is_airkiss)
    {
        is_airkiss = FALSE;
    }
    else
    {
        return ;
    }
    /* 13.加网成功之后，向10000端口广播发送udp报文，通告一键配置已经配置成功 */
    socket_num = socket(AF_INET, SOCK_DGRAM, 0);
    AIRKISS_PRINT("create skt %d: send udp broadcast to airkiss.\r\n", socket_num);

    memset(&addr, 0, sizeof(struct sockaddr_in));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(ONESHOT_AIRKISS_REMOTE_PORT);
    addr.sin_addr.s_addr = htonl(INADDR_BROADCAST);

    for (idx = 0; idx < ONESHOT_AIRKISS_REPLY_CNT_MAX; idx++)
    {
        if (tls_wifi_get_oneshot_flag())
        {
            break;
        }
        /* 发送结果为包含get_result所得random值的一个字节udp数据包 */
        sendto(socket_num, &random4reply, sizeof(random4reply), 0, (struct sockaddr*) &addr, sizeof(struct sockaddr_in));
        tls_os_time_delay(50);
    }

    closesocket(socket_num);

    return ;
}

//-------------------------------------------------------------------------

static void oneshot_airkiss_finish_new(u8 *ssid, u8 ssid_len, u8 *pwd, u8 pwd_len,  u8 *bssid, u8 randomnum)
{
    int ret =  - 1;

    random4reply = randomnum;
    is_airkiss = TRUE;
    {
        tls_netif_add_status_event(wm_oneshot_netif_status_event);
        tls_wifi_set_oneshot_flag(0);
        ret = tls_wifi_connect(ssid, ssid_len, pwd, pwd_len);
    }

    if (WM_SUCCESS != ret)
    {
        AIRKISS_PRINT("failed to connect net, airkiss join net failed.\r\n");
    }

    return ;
}

//-------------------------------------------------------------------------

int airkiss_step_mark = 0;

u8 airkiss_data[2][128];

u8 airkiss_pwd[65];
u8 airkiss_ssid[33];

static u8 crc8_new_tbl[256] =
{
  0x00, 0x5E, 0xBC, 0xE2, 0x61, 0x3F, 0xDD, 0x83, 0xC2, 0x9C, 0x7E, 0x20, 0xA3, 0xFD, 0x1F, 0x41, 
  0x9D, 0xC3, 0x21, 0x7F, 0xFC, 0xA2, 0x40, 0x1E, 0x5F, 0x01, 0xE3, 0xBD, 0x3E, 0x60, 0x82, 0xDC, 
  0x23, 0x7D, 0x9F, 0xC1, 0x42, 0x1C, 0xFE, 0xA0, 0xE1, 0xBF, 0x5D, 0x03, 0x80, 0xDE, 0x3C, 0x62, 
  0xBE, 0xE0, 0x02, 0x5C, 0xDF, 0x81, 0x63, 0x3D, 0x7C, 0x22, 0xC0, 0x9E, 0x1D, 0x43, 0xA1, 0xFF,
  0x46, 0x18, 0xFA, 0xA4, 0x27, 0x79, 0x9B, 0xC5, 0x84, 0xDA, 0x38, 0x66, 0xE5, 0xBB, 0x59, 0x07, 
  0xDB, 0x85, 0x67, 0x39, 0xBA, 0xE4, 0x06, 0x58, 0x19, 0x47, 0xA5, 0xFB, 0x78, 0x26, 0xC4, 0x9A, 
  0x65, 0x3B, 0xD9, 0x87, 0x04, 0x5A, 0xB8, 0xE6, 0xA7, 0xF9, 0x1B, 0x45, 0xC6, 0x98, 0x7A, 0x24, 
  0xF8, 0xA6, 0x44, 0x1A, 0x99, 0xC7, 0x25, 0x7B, 0x3A, 0x64, 0x86, 0xD8, 0x5B, 0x05, 0xE7, 0xB9, 
  0x8C, 0xD2, 0x30, 0x6E, 0xED, 0xB3, 0x51, 0x0F, 0x4E, 0x10, 0xF2, 0xAC, 0x2F, 0x71, 0x93, 0xCD, 
  0x11, 0x4F, 0xAD, 0xF3, 0x70, 0x2E, 0xCC, 0x92, 0xD3, 0x8D, 0x6F, 0x31, 0xB2, 0xEC, 0x0E, 0x50, 
  0xAF, 0xF1, 0x13, 0x4D, 0xCE, 0x90, 0x72, 0x2C, 0x6D, 0x33, 0xD1, 0x8F, 0x0C, 0x52, 0xB0, 0xEE, 
  0x32, 0x6C, 0x8E, 0xD0, 0x53, 0x0D, 0xEF, 0xB1, 0xF0, 0xAE, 0x4C, 0x12, 0x91, 0xCF, 0x2D, 0x73, 
  0xCA, 0x94, 0x76, 0x28, 0xAB, 0xF5, 0x17, 0x49, 0x08, 0x56, 0xB4, 0xEA, 0x69, 0x37, 0xD5, 0x8B, 
  0x57, 0x09, 0xEB, 0xB5, 0x36, 0x68, 0x8A, 0xD4, 0x95, 0xCB, 0x29, 0x77, 0xF4, 0xAA, 0x48, 0x16, 
  0xE9, 0xB7, 0x55, 0x0B, 0x88, 0xD6, 0x34, 0x6A, 0x2B, 0x75, 0x97, 0xC9, 0x4A, 0x14, 0xF6, 0xA8, 
  0x74, 0x2A, 0xC8, 0x96, 0x15, 0x4B, 0xA9, 0xF7, 0xB6, 0xE8, 0x0A, 0x54, 0xD7, 0x89, 0x6B, 0x35
};


u8 get_crc_8(u8 *ptr, u32 len)
{
    u8 crc8;
    u8 data;

    crc8 = 0;
    while (len-- != 0)
    {
        data =  *ptr++;
        crc8 = crc8_new_tbl[crc8 ^ data];
    }

    return crc8;
}

//-------------------------------------------------------------------------
static u8 gBackupMac[ETH_ALEN] = {0, 0, 0, 0, 0, 0};
static u32 gairkissbit[2] = {0, 0};
static u8 gSrcMac[ETH_ALEN] =
{
    0, 0, 0, 0, 0, 0
};
extern u8 tls_oneshot_is_ssid_crc_match(u8 crc, u8 *ssid, u8 *ssid_len);

void tls_oneshot_airkiss_change_channel(void)
{
    airkiss_step_mark =  - 1;
	memset(pakcontext, 0x00, sizeof(pakcontext));
}

static __inline u8 tls_compare_ether_addr(const u8 *addr1, const u8 *addr2)
{
    return !((addr1[0] == addr2[0]) && (addr1[1] == addr2[1]) && (addr1[2] == addr2[2]) &&  \
        (addr1[3] == addr2[3]) && (addr1[4] == addr2[4]) && (addr1[5] == addr2[5]));
}

//-------------------------------------------------------------------------

static __inline u8 tls_wifi_compare_mac_addr(u8 *macaddr)
{
    u8 tmpmacaddr[ETH_ALEN] =
    {
        0, 0, 0, 0, 0, 0
    };

    if (macaddr == NULL)
    {
        return 0;
    }

    if (tls_compare_ether_addr(gSrcMac, tmpmacaddr) == 0)
    {
        return 1;
    }

    if (tls_compare_ether_addr(gSrcMac, macaddr) == 0)
    {
        return 1;
    }
    return 0;
}

//-------------------------------------------------------------------------

typedef struct airkiss_data_st
{
    u32 seqnum[2];

    u8 baselenarray[2];
    u8 basesynccnt[2];
    u8 airkiss_sync_times;
    u8 airkiss_base_len;

    u8 akinfocnt[2];
    u8 akinfodata[2][4];
    u8 airkiss_total_len;
    u8 airkiss_total_index;
    u32 airkiss_all_data_bit[2];

    u8 airkiss_last_packet_len;
    u8 airkiss_ssid_crc;
    u8 airkiss_ssid_len;

    u8 airkiss_pwd_rd_index;
    u8 airkiss_pwd_len;
    u8 airkiss_pwd_crc;

    u8 akdatacnt[2];
    u8 akdata[2][6];
    u8 resv;
} airkiss_data_st;

void tls_airkiss_recv_new(u8 *pdata, u8 *data, u16 data_len)
{
    struct ieee80211_hdr *hdr = (struct ieee80211_hdr*)data;
    u8 *broadcast = NULL;
    u8 *SrcMac = NULL;
    airkiss_data_st *stairkissdata = (airkiss_data_st*)pdata;

    u8 index = 0;
    u8 isfromds = 0;
    u32 frm_len = 0;


    broadcast = ieee80211_get_DA(hdr);
    if (0 == is_broadcast_ether_addr(broadcast))
    {
        return ;
    }

    if (ieee80211_is_data_qos(hdr->frame_control))
    {
        frm_len = data_len - 2;
    }
    else
    {
        frm_len = data_len;
    }

    isfromds = ieee80211_has_fromds(hdr->frame_control);

    if (airkiss_step_mark ==  - 1)
    {
        memset(stairkissdata, 0, sizeof(*stairkissdata));
        memset(gSrcMac, 0, ETH_ALEN);
        airkiss_step_mark = 0;
    }

    SrcMac = ieee80211_get_SA(hdr);
    if (tls_wifi_compare_mac_addr(SrcMac) == 0)
    {
        return ;
    }

    switch (airkiss_step_mark)
    {
        case 0:
            /*同步*/
            //if (isfromds == 0)
            {
                if (frm_len <= 85)
                {
                    if (ieee80211_has_retry(hdr->frame_control) && (stairkissdata->seqnum[isfromds] == hdr->seq_ctrl))
                    {
                        break;
                    }
                    else
                    {
                        stairkissdata->seqnum[isfromds] = hdr->seq_ctrl;
                    }

                    AIRKISS_PRINT("isfromds:%d, MAC:"MACSTR",frm_len:%03d\n", isfromds, MAC2STR(SrcMac), frm_len);

                    if (stairkissdata->basesynccnt[isfromds] == 0)
                    {
                        if ((frm_len == 61) || (frm_len == 69) || (frm_len == 77) || (frm_len == 81))
                        {
                            stairkissdata->baselenarray[isfromds] = frm_len;
                            stairkissdata->basesynccnt[isfromds] = 1;
                            MEMCPY(gSrcMac, SrcMac, ETH_ALEN);
                            if (isfromds == 0){
                                MEMCPY(ak_bssid, hdr->addr1, ETH_ALEN);
                            }else{
                                MEMCPY(ak_bssid, hdr->addr2, ETH_ALEN);
                            }							
                            AIRKISS_PRINT("Src:" MACSTR ", tick:%d\n", MAC2STR(gSrcMac), tls_os_get_time());
                            tls_oneshot_switch_channel_tim_temp_stop();
                        }
                    }
                    else
                    {
                        if ((stairkissdata->baselenarray[isfromds] + stairkissdata->basesynccnt[isfromds]) == frm_len)
                        {
                            stairkissdata->basesynccnt[isfromds]++;
                        }
                        else
                        {
                            stairkissdata->basesynccnt[isfromds] = 0;
                            if ((frm_len == 61) || (frm_len == 69) || (frm_len == 77) || (frm_len == 81))
                            {
                                stairkissdata->baselenarray[isfromds] = frm_len;
                                stairkissdata->basesynccnt[isfromds] = 1;
                                MEMCPY(gSrcMac, SrcMac, ETH_ALEN);
                                if (isfromds == 0){
                                    MEMCPY(ak_bssid, hdr->addr1, ETH_ALEN);
                                }else{
                                    MEMCPY(ak_bssid, hdr->addr2, ETH_ALEN);
                                }								
                                tls_oneshot_switch_channel_tim_temp_stop();
                            }
                        }
                    }

                    if (stairkissdata->basesynccnt[isfromds] == 4)
                    {
                        AIRKISS_PRINT("fixed chan\n");
                        if (stairkissdata->airkiss_base_len)
                        {
                            if (stairkissdata->airkiss_base_len == (stairkissdata->baselenarray[isfromds] - 1))
                            {
                                stairkissdata->airkiss_sync_times++;
                            }
                            else
                            {
                                stairkissdata->airkiss_sync_times = 1;
                            }
                        }
                        else
                        {
                            stairkissdata->airkiss_sync_times = 1;
                        }

                        stairkissdata->airkiss_base_len = stairkissdata->baselenarray[isfromds] - 1;
                        stairkissdata->basesynccnt[isfromds] = 0;
                    }
                }
                else
                {
                    stairkissdata->basesynccnt[0] = stairkissdata->basesynccnt[1] = 0;
                }

                if ((stairkissdata->airkiss_sync_times >= 2) && stairkissdata->airkiss_base_len)
                {
                    airkiss_step_mark = 1;
                    stairkissdata->airkiss_sync_times = 0;
                    printf("airkiss: sync chan, base len:%d, tick:%d\n", stairkissdata->airkiss_base_len, tls_os_get_time());
                }
            }
            break;

        case 1:
            /*获取总数据长度和SSID CRC值*/
            if ((frm_len >= (stairkissdata->airkiss_base_len)) && (frm_len <= (0x3F + stairkissdata->airkiss_base_len)))
            {
                if (ieee80211_has_retry(hdr->frame_control) && (stairkissdata->seqnum[isfromds] == hdr->seq_ctrl))
                {
                    break;
                }
                else
                {
                    stairkissdata->seqnum[isfromds] = hdr->seq_ctrl;
                }

                /*get data len, crc*/
                if (stairkissdata->akinfocnt[isfromds] == 0)
                {
                    if (((frm_len - stairkissdata->airkiss_base_len) &0xF0) == 0x00)
                    {
                        stairkissdata->akinfodata[isfromds][0] = (frm_len - stairkissdata->airkiss_base_len);
                        stairkissdata->akinfocnt[isfromds] = 1;
                    }
                }
                else
                {
                    if (((stairkissdata->akinfodata[isfromds][stairkissdata->akinfocnt[isfromds] - 1] &0xF0) + 0x10) == ((frm_len -stairkissdata->airkiss_base_len) &0xF0))
                    {
                        stairkissdata->akinfodata[isfromds][stairkissdata->akinfocnt[isfromds]++] = (frm_len - stairkissdata->airkiss_base_len);
                    }
                    else
                    {
                        stairkissdata->akinfocnt[isfromds] = 0;
                        if (((frm_len - stairkissdata->airkiss_base_len) &0xF0) == 0x00)
                        {
                            stairkissdata->akinfodata[isfromds][0] = (frm_len - stairkissdata->airkiss_base_len);
                            stairkissdata->akinfocnt[isfromds] = 1;
                        }
                    }
                }

                if (stairkissdata->akinfocnt[isfromds] == 4)
                {
                    if (stairkissdata->airkiss_total_len)
                    {
                        if ((stairkissdata->airkiss_total_len == (((stairkissdata->akinfodata[isfromds][0] &0xF) << 4) |
                            (stairkissdata->akinfodata[isfromds][1] &0xF))) && (stairkissdata->airkiss_ssid_crc == (((stairkissdata->akinfodata[isfromds][2] &0xF) << 4) | (stairkissdata->akinfodata[isfromds][3] &0xF))))
                        {
                            stairkissdata->airkiss_sync_times++;
                        }
                        else
                        {
                            stairkissdata->airkiss_sync_times = 1;
                        }
                    }
                    else
                    {
                        stairkissdata->airkiss_total_len = ((stairkissdata->akinfodata[isfromds][0] &0xF) << 4) | (stairkissdata->akinfodata[isfromds][1] &0xF);
                        stairkissdata->airkiss_ssid_crc = ((stairkissdata->akinfodata[isfromds][2] &0xF) << 4) | (stairkissdata->akinfodata[isfromds][3] &0xF);
                        stairkissdata->airkiss_sync_times = 1;
                    }

                    stairkissdata->akinfocnt[isfromds] = 0;
                }

                if ((stairkissdata->airkiss_total_len > (32+1+64))||(stairkissdata->airkiss_total_len == 0))
                {
                    stairkissdata->airkiss_total_len = 0;
                    stairkissdata->airkiss_ssid_crc = 0;
                    stairkissdata->airkiss_sync_times = 1;
                }

                if (stairkissdata->airkiss_sync_times >= 2)
                {
                    if (memcmp(gBackupMac, gSrcMac, ETH_ALEN) == 0)
                    {
                        stairkissdata->airkiss_all_data_bit[0] = gairkissbit[0];
                        stairkissdata->airkiss_all_data_bit[1] = gairkissbit[1];
                    }
                    else
                    {
                        stairkissdata->airkiss_all_data_bit[0] = 0;
                        stairkissdata->airkiss_all_data_bit[1] = 0;
                        MEMCPY(gBackupMac, gSrcMac, ETH_ALEN);
                    }

                    AIRKISS_PRINT("ds:%d,  total_len:%d, ssid_crc:%x, tick:%d\n", isfromds, stairkissdata->airkiss_total_len, stairkissdata->airkiss_ssid_crc, tls_os_get_time());
                    {
                        airkiss_step_mark = 2;
                        stairkissdata->airkiss_sync_times = 0;
                        stairkissdata->akinfocnt[0] = stairkissdata->akinfocnt[1] = 0;
                        memset(stairkissdata->akinfodata, 0, sizeof(stairkissdata->akinfodata));
                    }
                }

            }
            break;
        case 2:
            // if (airkiss_step_mark == 2)	    /*获取PWD长度,PWD CRC*/
            {
                if ((frm_len >= (0x40 + stairkissdata->airkiss_base_len)) && (frm_len <= (0x7F + stairkissdata->airkiss_base_len)))
                {
                    if (ieee80211_has_retry(hdr->frame_control) && (stairkissdata->seqnum[isfromds] == hdr->seq_ctrl))
                    {
                        break;
                    }
                    else
                    {
                        stairkissdata->seqnum[isfromds] = hdr->seq_ctrl;
                    }

                    if (stairkissdata->akinfocnt[isfromds] == 0)
                    {
                        if (((frm_len - stairkissdata->airkiss_base_len) &0xF0) == 0x40)
                        {
                            stairkissdata->akinfodata[isfromds][0] = (frm_len - stairkissdata->airkiss_base_len);
                            stairkissdata->akinfocnt[isfromds] = 1;
                        }
                    }
                    else
                    {
                        if (((stairkissdata->akinfodata[isfromds][stairkissdata->akinfocnt[isfromds] - 1] &0xF0) + 0x10) == ((frm_len - stairkissdata->airkiss_base_len) &0xF0))
                        {
                            stairkissdata->akinfodata[isfromds][stairkissdata->akinfocnt[isfromds]++] = (frm_len - stairkissdata->airkiss_base_len);
                        }
                        else
                        {
                            stairkissdata->akinfocnt[isfromds] = 0;
                            if (((frm_len - stairkissdata->airkiss_base_len) &0xF0) == 0x40)
                            {
                                stairkissdata->akinfodata[isfromds][0] = (frm_len - stairkissdata->airkiss_base_len);
                                stairkissdata->akinfocnt[isfromds] = 1;
                            }
                        }
                    }

                    if (stairkissdata->akinfocnt[isfromds] == 4)
                    {
                        if ((stairkissdata->airkiss_pwd_len == (((stairkissdata->akinfodata[isfromds][0] &0xF) << 4) |(stairkissdata->akinfodata[isfromds][1] &0xF))) 
				&& (stairkissdata->airkiss_pwd_crc == (((stairkissdata->akinfodata[isfromds][2] &0xF) << 4) | (stairkissdata->akinfodata[isfromds][3] &0xF))))
                        {
                            stairkissdata->airkiss_sync_times++;
                        }
                        else
                        {
                            stairkissdata->airkiss_sync_times = 1;
                        }
                        stairkissdata->airkiss_pwd_len = ((stairkissdata->akinfodata[isfromds][0] &0xF) << 4) | (stairkissdata->akinfodata[isfromds][1] &0xF);
                        stairkissdata->airkiss_pwd_crc = ((stairkissdata->akinfodata[isfromds][2] &0xF) << 4) | (stairkissdata->akinfodata[isfromds][3] &0xF);

                        if (crc8_new_tbl[stairkissdata->airkiss_pwd_len] != stairkissdata->airkiss_pwd_crc)
                        {
                            stairkissdata->airkiss_sync_times = 1;
                        }

                        AIRKISS_PRINT("ds:%d, pwd_len:%d, pwd_crc:%x\n", isfromds, stairkissdata->airkiss_pwd_len, stairkissdata->airkiss_pwd_crc);
                        stairkissdata->akinfocnt[isfromds] = 0;
                    }
                    if (stairkissdata->airkiss_pwd_len > 64)
                    {
                        stairkissdata->airkiss_pwd_len = 0;
                        stairkissdata->airkiss_pwd_crc = 0;
                        stairkissdata->airkiss_sync_times = 1;
                    }

                    if (stairkissdata->airkiss_sync_times >= 2)
                    {
                        airkiss_data[0][stairkissdata->airkiss_total_len] = '\0';
                        airkiss_data[1][stairkissdata->airkiss_total_len] = '\0';
                        stairkissdata->airkiss_ssid_len = stairkissdata->airkiss_total_len - stairkissdata->airkiss_pwd_len - 1;
                        if ((stairkissdata->airkiss_ssid_len > 32) || (stairkissdata->airkiss_ssid_len == 0))
                        {
                            airkiss_step_mark =  - 1;
                        }
                        else
                        {
                            tls_oneshot_switch_channel_tim_stop(hdr);
                            stairkissdata->airkiss_total_index = stairkissdata->airkiss_total_len % 4 == 0 ? stairkissdata->airkiss_total_len / 4: stairkissdata->airkiss_total_len / 4+1;
                            stairkissdata->airkiss_last_packet_len = stairkissdata->airkiss_total_len % 4;
                            if (tls_oneshot_is_ssid_crc_match(stairkissdata->airkiss_ssid_crc, airkiss_ssid, &stairkissdata->airkiss_ssid_len))
                            {
                                MEMCPY(&airkiss_data[0][stairkissdata->airkiss_pwd_len + 1], airkiss_ssid, stairkissdata->airkiss_ssid_len);
                                MEMCPY(&airkiss_data[1][stairkissdata->airkiss_pwd_len + 1], airkiss_ssid, stairkissdata->airkiss_ssid_len);
                                airkiss_ssid[stairkissdata->airkiss_ssid_len] = '\0';
                                stairkissdata->airkiss_pwd_rd_index = (stairkissdata->airkiss_pwd_len + 1) % 4 == 0 ? (stairkissdata->airkiss_pwd_len + 1) / 4: (stairkissdata->airkiss_pwd_len + 1) / 4+1;								
                                AIRKISS_PRINT("airkiss_ssid:%s\n", airkiss_ssid);
                                AIRKISS_PRINT("airkiss_total_len:%d, %d, %d\n", stairkissdata->airkiss_total_len, stairkissdata->airkiss_pwd_len, stairkissdata->airkiss_ssid_len);
                            }

                            airkiss_step_mark = 3;
                        }
                        stairkissdata->airkiss_sync_times = 0;
                        stairkissdata->akinfocnt[0] = stairkissdata->akinfocnt[1] = 0;
                        memset(stairkissdata->akinfodata, 0, sizeof(stairkissdata->akinfodata));
                    }
                }
            }
            break;

        case 3:
            {
                if ((frm_len >= (0x80 + stairkissdata->airkiss_base_len)) && (frm_len <= (0x1FF + stairkissdata->airkiss_base_len)))
                {
                    if (ieee80211_has_retry(hdr->frame_control) && (stairkissdata->seqnum[isfromds] == hdr->seq_ctrl))
                    {
                        break;
                    }
                    else
                    {
                        stairkissdata->seqnum[isfromds] = hdr->seq_ctrl;
                    }

                    if (stairkissdata->akdatacnt[isfromds] < 2)
                    {
                        if (((frm_len - stairkissdata->airkiss_base_len) &0x180) == 0x80)
                        {
                            stairkissdata->akdata[isfromds][stairkissdata->akdatacnt[isfromds]++] = (frm_len - stairkissdata->airkiss_base_len) &0x7F;
                            if (stairkissdata->akdatacnt[isfromds] == 2)
                            {
                                index = stairkissdata->akdata[isfromds][1];
                                if ((index >= stairkissdata->airkiss_total_index) || (stairkissdata->airkiss_all_data_bit[isfromds]&(1UL << index)))
                                {
                                    stairkissdata->akdatacnt[isfromds] = 0; /*对于已接收或者序号超过总序号的，重新来收*/
                                    break;
                                }
                            }
                        }
                        else
                        {
                            stairkissdata->akdatacnt[isfromds] = 0;
                        }
                    }
                    else
                    {
                        if (0x100 == ((frm_len - stairkissdata->airkiss_base_len) &0x100))
                        {
                            stairkissdata->akdata[isfromds][stairkissdata->akdatacnt[isfromds]++] = (frm_len - stairkissdata->airkiss_base_len) &0xFF;
                        }
                        else
                        {
                            stairkissdata->akdatacnt[isfromds] = 0;
                            if (((frm_len - stairkissdata->airkiss_base_len) &0x180) == 0x80)
                            {
                                stairkissdata->akdata[isfromds][stairkissdata->akdatacnt[isfromds]++] = (frm_len - stairkissdata->airkiss_base_len) &0x7F;
                            }
                        }
                    }

                    if ((stairkissdata->akdatacnt[isfromds] == 6) || ((stairkissdata->akdatacnt[isfromds] > 2) 
                        && stairkissdata->airkiss_last_packet_len 
                        && ((stairkissdata->akdata[isfromds][1] + 1) == stairkissdata->airkiss_total_index) 
                        && (stairkissdata->akdatacnt[isfromds] == (2+stairkissdata->airkiss_last_packet_len))
                    ))
                    {
                        index = stairkissdata->akdata[isfromds][1];
                        if (stairkissdata->akdata[isfromds][0] == (get_crc_8(&stairkissdata->akdata[isfromds][1], (stairkissdata->akdatacnt[isfromds] - 1)) &0x7F))
                        {
                            AIRKISS_PRINT("crc:%x,ds:%d, index:%d, %d\n", stairkissdata->akdata[isfromds][0], isfromds, index, stairkissdata->airkiss_total_index);
                            MEMCPY(&airkiss_data[isfromds][index *4], &stairkissdata->akdata[isfromds][2], stairkissdata->akdatacnt[isfromds] - 2);
                            stairkissdata->airkiss_all_data_bit[isfromds] |= 1UL << index;
                            gairkissbit[isfromds] = stairkissdata->airkiss_all_data_bit[isfromds];
                        }
                        stairkissdata->akdatacnt[isfromds] = 0;
                    }
                }

                if (((stairkissdata->airkiss_all_data_bit[isfromds] == ((1 << stairkissdata->airkiss_total_index) - 1)) 
                        || (stairkissdata->airkiss_pwd_rd_index && (stairkissdata->airkiss_all_data_bit[isfromds] &((1 << stairkissdata->airkiss_pwd_rd_index) - 1)) == ((1 << stairkissdata->airkiss_pwd_rd_index) - 1))))
                {
                    airkiss_step_mark = 4;
                    AIRKISS_PRINT("airkiss_data:%s\n", airkiss_data[isfromds]);
                    if (stairkissdata->airkiss_pwd_len)
                    {
                        MEMCPY((char*)airkiss_pwd, (char*)airkiss_data[isfromds], stairkissdata->airkiss_pwd_len);
                        airkiss_pwd[stairkissdata->airkiss_pwd_len] = '\0';
                    }
                    else
                    {
                        airkiss_pwd[0] = '\0';
                    }

                    if (stairkissdata->airkiss_pwd_rd_index)
                    {
                        /*nothing to do*/
                    }
                    else
                    {
                        MEMCPY((char*)airkiss_ssid, (char*) &airkiss_data[isfromds][stairkissdata->airkiss_pwd_len + 1],  stairkissdata->airkiss_total_len - (stairkissdata->airkiss_pwd_len + 1));
                        airkiss_ssid[stairkissdata->airkiss_total_len - (stairkissdata->airkiss_pwd_len + 1)] = '\0';
                    }
                    AIRKISS_PRINT("airkiss total data len:%d, pwd len:%d, ssid len:%d\n", stairkissdata->airkiss_total_len, stairkissdata->airkiss_pwd_len, stairkissdata->airkiss_total_len - (stairkissdata->airkiss_pwd_len + 1));
                    printf("airkiss: pwd:%s, random:%d,ssid:%s, tick:%d\n", airkiss_pwd, airkiss_data[isfromds][stairkissdata->airkiss_pwd_len], airkiss_ssid, tls_os_get_time());
                    oneshot_airkiss_finish_new(airkiss_ssid, stairkissdata->airkiss_total_len - stairkissdata->airkiss_pwd_len - 1,airkiss_pwd, stairkissdata->airkiss_pwd_len, ak_bssid, airkiss_data[isfromds][stairkissdata->airkiss_pwd_len]);
                    airkiss_step_mark =  - 1;
                }
            }
            break;

        default:
            break;
    }
}

//-------------------------------------------------------------------------


void tls_airkiss_recv(u8 *data, u16 data_len)
{
    tls_airkiss_recv_new((u8*)pakcontext, data, data_len);
    return ;
}

void tls_airkiss_start(void)
{
    AIRKISS_PRINT("start airkiss oneshot config...\r\n");

#if AIRKISS_BSSID_CONNECT_ENABLE
    memset(ak_bssid, 0, ETH_ALEN);
#endif
    gairkissbit[0] = gairkissbit[1] = 0;
	memset(pakcontext, 0x00, sizeof(pakcontext));
    is_airkiss = FALSE;

    return ;
}

void tls_airkiss_stop(void)
{
    AIRKISS_PRINT("stop airkiss oneshot config...\r\n");

    return ;
}
#endif

