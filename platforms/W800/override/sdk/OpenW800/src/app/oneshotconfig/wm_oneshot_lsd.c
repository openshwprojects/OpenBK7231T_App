
#include "wm_oneshot_lsd.h"
#include "wm_mem.h"



#if LSD_ONESHOT_DEBUG
#define LSD_ONESHOT_DBG 	printf
#else
#define LSD_ONESHOT_DBG(s, ...)
#endif


#define LSD_GUIDE_DATUM			1
#define LSD_DATA_OFFSET			20

#define LSD_REPLY_PORT			65534
#define LSD_REPLY_MAX_CNT		20

#define LSD_DATA_MAX			256



struct lsd_data_t{
	u8 data[LSD_DATA_MAX];
	u8 used[LSD_DATA_MAX];
};

struct lsd_data_coding_t{
	u8 data1;
	u8 data2;
	u8 seq;
	u8 crc;
};


lsd_printf_fn lsd_printf = NULL;

static u8 *lsd_scan_bss;

const u8 lsd_dst_addr[3] = {0x01,0x00,0x5e};
u8 lsd_last_num[2] = {0,0};
u16 lsd_head[2][4] = {{0,0,0,0},{0,0,0,0}};
u16 lsd_byte[2][4] = {{0,0,0,0},{0,0,0,0}};
u8 lsd_state = 0;
u16	lsd_data_datum = 0; 
u8 lsd_head_cnt[2] = {0,0};
u8 lsd_byte_cnt[2] = {0,0};
u8 lsd_sync_cnt = 0;
u8 lsd_src_mac[6] = {0};
u8 lsd_data_cnt = 0;
u16 lsd_last_seq[2] = {0,0};
u16 lsd_last_len = 0;
u8 lsd_temp_lock = 0;


struct lsd_data_t *lsd_data = NULL;
struct lsd_param_t *lsd_param = NULL;


u8 lsd_crc_value = 0;
const u8 lsd_crc_table[256] = {
		0x0 ,0x91 ,0xe3 ,0x72 ,0x7 ,0x96 ,0xe4 ,0x75 ,0xe ,0x9f ,0xed ,0x7c ,0x9 ,
		0x98 ,0xea ,0x7b ,0x1c ,0x8d ,0xff ,0x6e ,0x1b ,0x8a ,0xf8 ,0x69 ,0x12 ,0x83 ,
		0xf1 ,0x60 ,0x15 ,0x84 ,0xf6 ,0x67 ,0x38 ,0xa9 ,0xdb ,0x4a ,0x3f ,0xae ,0xdc ,
		0x4d ,0x36 ,0xa7 ,0xd5 ,0x44 ,0x31 ,0xa0 ,0xd2 ,0x43 ,0x24 ,0xb5 ,0xc7 ,0x56 ,
		0x23 ,0xb2 ,0xc0 ,0x51 ,0x2a ,0xbb ,0xc9 ,0x58 ,0x2d ,0xbc ,0xce ,0x5f ,0x70 ,
		0xe1 ,0x93 ,0x2 ,0x77 ,0xe6 ,0x94 ,0x5 ,0x7e ,0xef ,0x9d ,0xc ,0x79 ,0xe8 ,0x9a ,
		0xb ,0x6c ,0xfd ,0x8f ,0x1e ,0x6b ,0xfa ,0x88 ,0x19 ,0x62 ,0xf3 ,0x81 ,0x10 ,0x65 ,
		0xf4 ,0x86 ,0x17 ,0x48 ,0xd9 ,0xab ,0x3a ,0x4f ,0xde ,0xac ,0x3d ,0x46 ,0xd7 ,0xa5 ,
		0x34 ,0x41 ,0xd0 ,0xa2 ,0x33 ,0x54 ,0xc5 ,0xb7 ,0x26 ,0x53 ,0xc2 ,0xb0 ,0x21 ,0x5a ,
		0xcb ,0xb9 ,0x28 ,0x5d ,0xcc ,0xbe ,0x2f ,0xe0 ,0x71 ,0x3 ,0x92 ,0xe7 ,0x76 ,0x4 ,
		0x95 ,0xee ,0x7f ,0xd ,0x9c ,0xe9 ,0x78 ,0xa ,0x9b ,0xfc ,0x6d ,0x1f ,0x8e ,0xfb ,
		0x6a ,0x18 ,0x89 ,0xf2 ,0x63 ,0x11 ,0x80 ,0xf5 ,0x64 ,0x16 ,0x87 ,0xd8 ,0x49 ,
		0x3b ,0xaa ,0xdf ,0x4e ,0x3c ,0xad ,0xd6 ,0x47 ,0x35 ,0xa4 ,0xd1 ,0x40 ,0x32 ,
		0xa3 ,0xc4 ,0x55 ,0x27 ,0xb6 ,0xc3 ,0x52 ,0x20 ,0xb1 ,0xca ,0x5b ,0x29 ,0xb8 ,
		0xcd ,0x5c ,0x2e ,0xbf ,0x90 ,0x1 ,0x73 ,0xe2 ,0x97 ,0x6 ,0x74 ,0xe5 ,0x9e ,0xf ,
		0x7d ,0xec ,0x99 ,0x8 ,0x7a ,0xeb ,0x8c ,0x1d ,0x6f ,0xfe ,0x8b ,0x1a ,0x68 ,0xf9 ,
		0x82 ,0x13 ,0x61 ,0xf0 ,0x85 ,0x14 ,0x66 ,0xf7 ,0xa8 ,0x39 ,0x4b ,0xda ,0xaf ,0x3e ,
		0x4c ,0xdd ,0xa6 ,0x37 ,0x45 ,0xd4 ,0xa1 ,0x30 ,0x42 ,0xd3 ,0xb4 ,0x25 ,0x57 ,0xc6 ,
		0xb3 ,0x22 ,0x50 ,0xc1 ,0xba ,0x2b ,0x59 ,0xc8 ,0xbd ,0x2c ,0x5e ,0xcf };


void lsd_crc8_init(u8 data)
{
	lsd_crc_value = data;
}

void lsd_crc8_update(u8 data)
{
	lsd_crc_value = lsd_crc_table[data ^ lsd_crc_value];
}

u8 lsd_crc8_get(void)
{
	return lsd_crc_value;
}

u8 lsd_crc8_calc(u8 *buf, u16 len)
{
	u16 i;
	
	lsd_crc8_init(0);
	for(i=0; i<len; i++)
	{
		lsd_crc8_update(buf[i]);
	}

	return lsd_crc8_get();
}

static void tls_find_ssid_nonascII_pos_and_count(u8 *ssid, u8 ssid_len, u8 *nonascii_cnt)
{
    int i = 0;
    int cnt = 0;

    if (ssid == NULL)
    {
    	return;
    }

    for (i = 0; i < ssid_len; i++)
    {
        if ( ssid[i] >= 0x80 )
        {
            cnt++;
        }
    }

    if (nonascii_cnt)
    {   
        *nonascii_cnt = cnt;
    }
}


static int lsd_ssid_bssid_crc_match(u8 ssidCrc, u8 bssidCrc, u8 *ssidLen, u8 *ssid,  u8 *bssid)
{
	int i = 0;
	u8 non_asciicnt = 0;
	struct tls_scan_bss_t *bss = NULL;	

	bss = (struct tls_scan_bss_t *)lsd_scan_bss;

	if(bss == NULL)
	{
		return -1;
	}

	for (i = 0; i < bss->count; i++)
	{
		if (bssidCrc == lsd_crc8_calc(bss->bss[i].bssid, 6))
		{
			non_asciicnt = 0;
			tls_find_ssid_nonascII_pos_and_count(ssid, *ssidLen, &non_asciicnt);
			if ((*ssidLen ==  bss->bss[i].ssid_len)
				&& (ssidCrc == lsd_crc8_calc(bss->bss[i].ssid, bss->bss[i].ssid_len)))
			{
				if(ssid != NULL)
				{
					memcpy(ssid, bss->bss[i].ssid, bss->bss[i].ssid_len);
				}
				memcpy(bssid, bss->bss[i].bssid, 6);
				return 0;
			}
			else if (non_asciicnt)
			{
				if(*ssidLen !=  bss->bss[i].ssid_len)
				{
					memcpy(ssid, bss->bss[i].ssid, bss->bss[i].ssid_len);
					*ssidLen = bss->bss[i].ssid_len;
				}
				memcpy(bssid, bss->bss[i].bssid, 6);
				return 0;
			}
		}
	}

	return -1;
}

int tls_lsd_recv(u8 *buf, u16 data_len)
{
    struct ieee80211_hdr *hdr = (struct ieee80211_hdr*)buf;
	struct lsd_data_coding_t data_coding;
    u8 *multicast = NULL;
    u8 *SrcMac = NULL;
	u16 i;
	u8 totalCrc, totalLen, pwdLen, ssidLen, ssidCrc, bssidCrc, pwdCrc, userLen;
	int ret;
	u16 frm_len;
	u16 guide_len;
	u8 tods = 0;
	u32 crcValue;
	if (NULL == lsd_data || NULL == lsd_param)
	{
		return -1;
	}
	multicast = ieee80211_get_DA(hdr);

	if(0 == ieee80211_has_tods(hdr->frame_control))
	{
		return LSD_ONESHOT_CONTINUE;
	}
	//for LSD only tods
    if (ieee80211_is_data_qos(hdr->frame_control))
    {
        frm_len = data_len - 2;
    }
	else
	{
		frm_len = data_len;
	}
	
	tods = ieee80211_has_tods(hdr->frame_control);
	SrcMac = ieee80211_get_SA(hdr);
	
	if(memcmp(multicast, lsd_dst_addr, 3))
	{
		return LSD_ONESHOT_CONTINUE;
	}

	switch(lsd_state)
	{
		case 0:
			if ((frm_len < 60) || (frm_len > 86))
			{
				return LSD_ONESHOT_CONTINUE;
			}
					
			if(is_zero_ether_addr(lsd_src_mac))
			{
				memcpy(lsd_src_mac, SrcMac, 6);
				lsd_head_cnt[0] = lsd_head_cnt[1] = 0;
				lsd_sync_cnt = 0;
				lsd_last_seq[0] = lsd_last_seq[1] = 0;
				lsd_temp_lock = 0;
				memset(lsd_head, 0, sizeof(lsd_head));
			}
			else
			{
				if(memcmp(lsd_src_mac, SrcMac, 6))
				{
					memcpy(lsd_src_mac, SrcMac, 6);
					lsd_head_cnt[0] = lsd_head_cnt[1] = 0;
					lsd_sync_cnt = 0;
					lsd_last_seq[0] = lsd_last_seq[1] = 0;
					memset(lsd_head, 0, sizeof(lsd_head));
				}else{
					if(lsd_printf)
						lsd_printf("tods:%d,%d,"MACSTR"\n", tods, frm_len, MAC2STR(SrcMac));
				}
			}

			if (ieee80211_has_retry(hdr->frame_control) && (lsd_last_seq[tods] == hdr->seq_ctrl))
			{
				return LSD_ONESHOT_CONTINUE;
			}
			lsd_last_seq[tods] = hdr->seq_ctrl;

			lsd_head[tods][lsd_head_cnt[tods]] = frm_len;

			if(lsd_head_cnt[tods] > 0)
			{
				if(((lsd_head[tods][lsd_head_cnt[tods]]+1) != lsd_head[tods][lsd_head_cnt[tods]-1])
					&& ((lsd_head[tods][lsd_head_cnt[tods]]-3) != lsd_head[tods][lsd_head_cnt[tods]-1]))
				{
					lsd_temp_lock = 0;
					lsd_head_cnt[tods] = 0;
					lsd_head[tods][0] = frm_len;
				}else{				
					lsd_temp_lock = 1;
				}
			}
			lsd_head_cnt[tods] ++;

			if(lsd_head_cnt[tods] >= 4)
			{
				lsd_sync_cnt ++;
				lsd_head_cnt[tods] = 0;
			}
	
			if(lsd_sync_cnt >= 1)
			{
				guide_len = lsd_head[tods][0];		
				for(i=1; i<=3; i++)
				{
					if(guide_len > lsd_head[tods][i])
						guide_len = lsd_head[tods][i];								//取出同步头中最小值					
				}
				lsd_state = 1;														//同步完成, 锁定源MAC和信道
				lsd_data_datum = guide_len - LSD_GUIDE_DATUM + LSD_DATA_OFFSET;		//获取到基准长度
				if(lsd_printf)
					lsd_printf("lsd lock:%d\n", lsd_data_datum);	

				printf("SRC MAC:%02X:%02X:%02X:%02X:%02X:%02X\n", 
					lsd_src_mac[0],lsd_src_mac[1],lsd_src_mac[2],lsd_src_mac[3],lsd_src_mac[4],lsd_src_mac[5]);
				return LSD_ONESHOT_CHAN_LOCKED;
			}
			if(lsd_temp_lock == 1)
			{
				return LSD_ONESHOT_CHAN_TEMP_LOCKED;
			}
			break;

		case 1:
			if((frm_len >= 1024) || (frm_len < lsd_data_datum))
			{
				return LSD_ONESHOT_CONTINUE;
			}

			if(memcmp(lsd_src_mac, SrcMac, 6))
			{
				return LSD_ONESHOT_CONTINUE;
			}
				
			if (ieee80211_has_retry(hdr->frame_control) && (lsd_last_seq[tods] == hdr->seq_ctrl))
			{
				return LSD_ONESHOT_CONTINUE;
			}
			lsd_last_seq[tods] = hdr->seq_ctrl;

			if(lsd_last_num[tods] != multicast[5])
			{
				memset((u8 *)&lsd_byte[tods][0], 0, 4);
				lsd_byte_cnt[tods] = 0;
				lsd_last_num[tods] = multicast[5];
			}

			lsd_byte[tods][lsd_byte_cnt[tods]] = frm_len - lsd_data_datum;
			if((lsd_byte_cnt[tods]==0) && (lsd_byte[tods][0]>=256))
			{
				lsd_byte_cnt[tods] = 0;
			}
			else if((lsd_byte_cnt[tods]==1) && (0x100!=(lsd_byte[tods][1]&0x300)))
			{
				lsd_byte_cnt[tods] = 0;
			}
			else if((lsd_byte_cnt[tods]==2) && (lsd_byte[tods][2]>=256))
			{
				lsd_byte_cnt[tods] = 0;
			}
			else if((lsd_byte_cnt[tods]==3) && (0x200!=(lsd_byte[tods][3]&0x300)))
			{
				lsd_byte_cnt[tods] = 0;
			}
			else
			{
				lsd_byte_cnt[tods] ++;
			}

			if(lsd_byte_cnt[tods] >= 4)
			{	
				data_coding.data1 = lsd_byte[tods][0]&0xFF;
				data_coding.crc = lsd_byte[tods][1]&0xFF;
				data_coding.data2 = lsd_byte[tods][2]&0xFF;
				data_coding.seq = lsd_byte[tods][3]&0xFF;
				if(lsd_data->used[data_coding.seq<<1] == 0)
				{
					crcValue = lsd_crc8_calc((u8 *)&data_coding, 3);
					if(data_coding.crc == (u8)crcValue)
					{
						if(lsd_printf)
							lsd_printf("%d\n", data_coding.seq);
						lsd_data->data[data_coding.seq<<1] = data_coding.data1;
						lsd_data->used[data_coding.seq<<1] = 1;
						lsd_data_cnt ++;
						lsd_data->data[(data_coding.seq<<1)+1] = data_coding.data2;
						lsd_data->used[(data_coding.seq<<1)+1] = 1;	
						lsd_data_cnt ++;
						if(lsd_data_cnt >= LSD_DATA_MAX)
						{
							lsd_data_cnt = 0;
							memset((u8 *)lsd_data, 0, sizeof(struct lsd_data_t));
							return LSD_ONESHOT_ERR;
						}
					}
				}
				lsd_byte_cnt[tods] = 0;
			}

			if(lsd_data->used[0] && lsd_data->used[1] && lsd_data->used[2])
			{
				totalLen = lsd_data->data[0];
				pwdLen = lsd_data->data[1];
				ssidLen = lsd_data->data[2];
				if((ssidLen > 32) || (pwdLen > 64))
				{
					lsd_data_cnt = 0;
					memset((u8 *)lsd_data, 0, sizeof(struct lsd_data_t));
					return LSD_ONESHOT_ERR;
				}
				if((pwdLen==0) && (ssidLen==0) && (totalLen<=2)) /*total len wrong*/
				{
					if(lsd_printf)
						lsd_printf("totalLen:%d, ssidLen:%d, pwdLen:%d, err\n", totalLen, ssidLen, pwdLen);

					lsd_data_cnt = 0;
					memset((u8 *)lsd_data, 0, sizeof(struct lsd_data_t));
					return LSD_ONESHOT_CONTINUE;					
				}
				else if((ssidLen>0) && (pwdLen>0))
				{
					if(totalLen < pwdLen + ssidLen + 5) /*total len wrong*/
					{
						if(lsd_printf)
							lsd_printf("totalLen:%d, ssidLen:%d, pwdLen:%d, err\n", totalLen, ssidLen, pwdLen);

						lsd_data_cnt = 0;
						memset((u8 *)lsd_data, 0, sizeof(struct lsd_data_t));
						return LSD_ONESHOT_CONTINUE; 
					}
				}
				else if((ssidLen>0) && (pwdLen==0))
				{
					if(totalLen < pwdLen + ssidLen + 4)/*total len wrong*/
					{
						if(lsd_printf)
							lsd_printf("totalLen:%d, ssidLen:%d, pwdLen:%d, err\n", totalLen, ssidLen, pwdLen);

						lsd_data_cnt = 0;
						memset((u8 *)lsd_data, 0, sizeof(struct lsd_data_t));
						return LSD_ONESHOT_CONTINUE; 
					}					
				}
				else if((ssidLen==0) && (pwdLen>0)) /*ssid len wrong*/
				{
					if(lsd_printf)
						lsd_printf("ssidLen:%d, pwdLen:%d, err\n", ssidLen, pwdLen);

					lsd_data_cnt = 0;
					memset((u8 *)lsd_data, 0, sizeof(struct lsd_data_t));
					return LSD_ONESHOT_CONTINUE;		
				}
				else if((ssidLen>32) || (pwdLen>64))
				{
					if(lsd_printf)
						lsd_printf("ssidLen:%d, pwdLen:%d, err\n", ssidLen, pwdLen);

					lsd_data_cnt = 0;
					memset((u8 *)lsd_data, 0, sizeof(struct lsd_data_t));
					return LSD_ONESHOT_CONTINUE;
				}	
				
				if(lsd_data_cnt >= totalLen + 2)
				{
					if(lsd_printf)
						lsd_printf("get all\n");
					totalCrc = lsd_data->data[totalLen+1];
					if(totalCrc != lsd_crc8_calc(&lsd_data->data[0], totalLen+1))
					{
						if(lsd_printf)
							lsd_printf("totalCrc err\n");

						lsd_data_cnt = 0;
						memset((u8 *)lsd_data, 0, sizeof(struct lsd_data_t));
						return LSD_ONESHOT_CONTINUE;
					}
				
					if((ssidLen==0) && (pwdLen==0))				//only userData
					{
						lsd_param->ssid_len = 0;
						lsd_param->pwd_len = 0;
						lsd_param->user_len = totalLen - 2;
						if(lsd_param->user_len > 128)
						{
							lsd_data_cnt = 0;
							memset((u8 *)lsd_data, 0, sizeof(struct lsd_data_t));
							return LSD_ONESHOT_ERR;
						}
						memcpy(lsd_param->user_data, &lsd_data->data[3], lsd_param->user_len);	
						if(lsd_printf)
							lsd_printf("user data:%s\n", lsd_param->user_data);
						return LSD_ONESHOT_COMPLETE;
					}
											
					bssidCrc = lsd_data->data[3];
					if(pwdLen > 0)
					{
						memcpy(lsd_param->pwd, &lsd_data->data[4], pwdLen);
						memcpy(lsd_param->ssid, &lsd_data->data[5+pwdLen], ssidLen);
						ssidCrc = lsd_data->data[5+ssidLen+pwdLen];	
						lsd_param->user_len = totalLen - pwdLen - ssidLen - 5;	
						if(lsd_param->user_len > 128)
						{
							lsd_data_cnt = 0;
							memset((u8 *)lsd_data, 0, sizeof(struct lsd_data_t));
							return LSD_ONESHOT_ERR;
						}
						memcpy(lsd_param->user_data, &lsd_data->data[6+ssidLen+pwdLen], lsd_param->user_len);
					}
					else
					{
						memcpy(lsd_param->ssid, &lsd_data->data[4+pwdLen], ssidLen);
						ssidCrc = lsd_data->data[4+ssidLen+pwdLen];
						lsd_param->user_len = totalLen - ssidLen - 4;	
						if(lsd_param->user_len > 128)
						{
							lsd_data_cnt = 0;
							memset((u8 *)lsd_data, 0, sizeof(struct lsd_data_t));
							return LSD_ONESHOT_ERR;
						}
						memcpy(lsd_param->user_data, &lsd_data->data[5+ssidLen], lsd_param->user_len);
					}	
					lsd_param->ssid_len = ssidLen;
					lsd_param->pwd_len = pwdLen;
					lsd_param->total_len = totalLen;
					if(lsd_printf)
						lsd_printf("user data:%s\n", lsd_param->user_data);
					if(lsd_printf)
						lsd_printf("ssidLen:%d, ssidCrc:%02X, bssidCrc:%02X\n", ssidLen, ssidCrc, bssidCrc);
					lsd_ssid_bssid_crc_match(ssidCrc, bssidCrc, &ssidLen, lsd_param->ssid, lsd_param->bssid);
					lsd_param->ssid_len = ssidLen;
					if(lsd_printf)
						lsd_printf("bssid:%02X%02X%02X%02X%02X%02X\n", lsd_param->bssid[0], lsd_param->bssid[1], lsd_param->bssid[2]
						, lsd_param->bssid[3], lsd_param->bssid[4], lsd_param->bssid[5]);	
					return LSD_ONESHOT_COMPLETE;
				}	//have no userData
				else if(ssidLen > 0)
				{
					if(pwdLen > 0)
					{
						userLen = totalLen - pwdLen - ssidLen - 5;
						if(0 == lsd_data->used[5+ssidLen+pwdLen])
						{
							return LSD_ONESHOT_CONTINUE;
						}
						ssidCrc = lsd_data->data[5+ssidLen+pwdLen];
					}
					else
					{
						userLen = totalLen - ssidLen - 4;
						if(0 == lsd_data->used[4+ssidLen+pwdLen])
						{
							return LSD_ONESHOT_CONTINUE;
						} 
						ssidCrc = lsd_data->data[4+ssidLen+pwdLen];
					}
					if(userLen > 0)					//have userData, must recv all
					{
						return LSD_ONESHOT_CONTINUE;
					}
					if(lsd_data->used[3])			//bssidCrc
					{
						bssidCrc = lsd_data->data[3];
						if(pwdLen > 0)
						{
							if(0 == lsd_data->used[4+pwdLen])
							{
								return LSD_ONESHOT_CONTINUE;
							}
							pwdCrc = lsd_data->data[4+pwdLen];
							for(i=0; i<pwdLen; i++)
							{
								if(lsd_data->used[4+i])
								{
									lsd_param->pwd[i] = lsd_data->data[4+i];
								}
								else
								{
									break;
								}
							}
							if(i != pwdLen)
							{
								return LSD_ONESHOT_CONTINUE;
							}
							if(pwdCrc != lsd_crc8_calc(&lsd_data->data[4], pwdLen))
							{
								if(lsd_printf)
									lsd_printf("pwdCrc err\n");
								lsd_data_cnt = 0;
								memset((u8 *)lsd_data, 0, sizeof(struct lsd_data_t));
								memset(lsd_param->pwd, 0, 65);
								return LSD_ONESHOT_CONTINUE;								
							}
						}
						ret = lsd_ssid_bssid_crc_match(ssidCrc, bssidCrc, &ssidLen, lsd_param->ssid,  lsd_param->bssid);
						if(ret == 0)
						{
							if(lsd_printf)
								lsd_printf("lsd_ssid_bssid_crc_match sucess\n");
							lsd_param->ssid_len = ssidLen;
							lsd_param->pwd_len = pwdLen;
							lsd_param->total_len = totalLen;
							return LSD_ONESHOT_COMPLETE;
						}
					}
				}
			}			
			break;
	}
	return LSD_ONESHOT_CONTINUE;
}

void tls_lsd_init(u8 *scanBss)
{
	if (NULL == lsd_data)
	{
		lsd_data = (struct lsd_data_t *)tls_mem_alloc(sizeof(struct lsd_data_t));
		if (lsd_data)
		{
			memset((u8 *)lsd_data, 0, sizeof(struct lsd_data_t));
		}
		else
		{
			if(lsd_printf)
				lsd_printf("lsd data malloc failed\n");
		}
	}
	else
	{
		memset((u8 *)lsd_data, 0, sizeof(struct lsd_data_t));
	}

	if (NULL == lsd_param)
	{
		lsd_param = (struct lsd_param_t *)tls_mem_alloc(sizeof(struct lsd_param_t));
		if (lsd_param)
		{
			memset((u8 *)lsd_param, 0, sizeof(struct lsd_param_t));
		}
		else
		{
			if(lsd_printf)
				lsd_printf("lsd param malloc failed\n");
		}
	}
	else
	{
		memset((u8 *)lsd_param, 0, sizeof(struct lsd_param_t));
	}

	
	memset(lsd_head, 0, sizeof(lsd_head));
	memset(lsd_byte, 0, sizeof(lsd_byte));
	memset(lsd_src_mac, 0, 6);

	memset(lsd_last_num, 0, sizeof(lsd_last_num));
	lsd_temp_lock = 0;
	lsd_state = 0;
	lsd_data_datum = 0; 
	memset(lsd_head_cnt, 0, sizeof(lsd_head_cnt));
	memset(lsd_byte_cnt, 0, sizeof(lsd_byte_cnt));
	lsd_sync_cnt = 0;
	lsd_data_cnt = 0;
	memset(lsd_last_seq, 0, sizeof(lsd_last_seq));
	lsd_scan_bss = scanBss;

	if(lsd_printf)
		lsd_printf("tls_lsd_init\n");
}


void tls_lsd_deinit(void)
{
	if (lsd_data)
	{
		tls_mem_free(lsd_data);
		lsd_data = NULL;
	}

	if (lsd_param)
	{
		tls_mem_free(lsd_param);
		lsd_param = NULL;
	}
}

