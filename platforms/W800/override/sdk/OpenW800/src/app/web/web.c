/*-------------------------------------------------------------------------
Copyright (c) 2014 Winner Micro Electronic Design Co., Ltd.
File name:	web.c
Author:		
Version:		
Date:		
Description:	
Others:		
  
Revision History:	
Who         When          What
--------    ----------    ----------------------------------------------
zhangwl    2018.07.18  Simplify the file
-------------------------------------------------------------------------*/
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "wm_params.h"
#include "wm_wifi.h"
#include "wm_cmdp.h"
//#include "typedef.h"
#include "os.h"
//#include "hal.h"
//#include "drv.h"
//#include "parm.h"
//#include "sys_def.h"
//#include "net.h"
#include "httpd.h"
#include "utils.h"
#include "wm_watchdog.h"
#include "wm_wifi_oneshot.h"


#define PRODUCT_MANUFACTOR "Beijing Winner Microelectronics Co., Ltd."

extern const u8 SysCreatedDate[];
extern const u8 SysCreatedTime[];

typedef struct _ID_VALUE{
	char *IdName;
	u8 tableid;
	u8 Idlen;
	u8 Value_Offset;
} ID_VALUE;

#define DNS_Dns_Id 1
#define DNS_Gate_Id   2
#define Web_Sub_Id    3
#define Web_Ip_Id       4
#define Web_Ssid_Id   5
#define Web_Encry_Id   6
#define KeyType_Id        7
#define Dhcp_Id              8
#define Web_Auto_Id      9
#define Web_Protocol_Id 10
#define Web_Cs_Id           11
#define Web_Domain_Id 12
#define Web_Port_Id 13
#define Web_Key_Id 14
#define Wep_KeyIndex_Id 15
#define Wep_KeyIndex2_Id 16
#define Wep_KeyIndex3_Id 17
#define Wep_KeyIndex4_Id 18
#define Web_Mode_Id    19
#define Web_AdhocAuto_Id 20
#define Web_Bg_Id    21
#define Web_Rate_Id   22
#define Web_Bssid_Id  23
#define Web_Channel_Id   24
#define Web_ChannlListCh1_Id 25
#define Web_ChannlListCh2_Id 26
#define Web_ChannlListCh3_Id 27
#define Web_ChannlListCh4_Id 28
#define Web_ChannlListCh5_Id 29
#define Web_ChannlListCh6_Id 30
#define Web_ChannlListCh7_Id 31
#define Web_ChannlListCh8_Id 32
#define Web_ChannlListCh9_Id 33
#define Web_ChannlListCh10_Id 34
#define Web_ChannlListCh11_Id 35
#define Web_ChannlListCh12_Id 36
#define Web_ChannlListCh13_Id 37
#define Web_ChannlListCh14_Id 38
#define Web_WebPort_Id  39
#define Web_Baud_Id  40
#define Web_Check_Id  41
#define Web_StopSize_Id 42
#define Web_DataSize_Id 43
#define Web_EscChar_Id  44
#define Web_EscP_Id 45
#define Web_EscDataP_Id 46
#define Web_EscDataL_Id 47
#define Web_GPIO1_Id  48
#define Web_CmdMode_Id 49
#define Web_BssidEable_Id  50
#define Web_PassWord_Id   51
#define Web_TCP_TimeOut_id 52
#define Web_SysInfo_Macaddr 53
#define Web_SysInfo_HardVer 54
#define Web_SysInfo_FirmVer 55
#define Web_SysInfo_RelTime 56
#define Web_SiteInfo_Copyright 57
#define Web_Try_times_id     58
#define Web_roam_id  59
#define Web_Autocreateadhoc_Id 60
#define Web_DnsName_Id 61
#define Web_SsidBroadcast_Id 62
#define Web_powersave_id  63
#define Web_scan_id  64
#define Web_WebEncry_id  65
#define Web_AutoHidden_Id      66

#define ENCRYPT_TYPE_OPEN_SYS  0///open
#define ENCRYPT_TYPE_WEP_64_BITS  1
#define ENCRYPT_TYPE_WEP_128_BITS  2

//extern u8* wpa_supplicant_get_mac(void);

const ID_VALUE web_id_tbl[] = {
   {
	"Dns",	
	DNS_Dns_Id,
	16,
	9,
    },
   {
	"Gate",	
	DNS_Gate_Id,
	17,
	8,
    },    
   {
   	"Sub",
	Web_Sub_Id,
	16,
	13,
   },
    {
   	"Ip",
	Web_Ip_Id,
	15,
	10,
   },
    {
   	"Ssid",
	Web_Ssid_Id,
	17,
	4,
   },
   {
   	"Encry",
	Web_Encry_Id,
	31,
	0,
	
   },
   {
   	"KeyType",
   	KeyType_Id,
   	15,
   	0,
   },
   {
   	"Dhcp",
	Dhcp_Id,
	10,
	0,
		
   },
  {
  	"Auto",
	Web_Auto_Id,
	10,
	0,
  },
  {
  	"Protocol",
	Web_Protocol_Id,
	16,
	0,
  },
  {
  	"Cs",
	Web_Cs_Id,
	11,
	0,
  },
  {
  	"Domain",
	Web_Domain_Id,
	19,
	11,
  },
  {
  	"Port",
	Web_Port_Id,
	17,
	5,
  },
  {
       "Key",
	 Web_Key_Id,
	 16,
	 0,
  },
   {
       "KeyIndex1",
	 Wep_KeyIndex_Id,
	 15,
	 0,
   },  
   {
       "KeyIndex2",
	 Wep_KeyIndex2_Id,
	 15,
	 0,
   },     {
       "KeyIndex3",
	 Wep_KeyIndex3_Id,
	 15,
	 0,
   },     
   {
       "KeyIndex4",
	 Wep_KeyIndex4_Id,
	 15,
	 0,
   },
   {
   	"Mode",
	Web_Mode_Id,
	30,
	0,
   },
   {
   	"Bg",
	Web_Bg_Id,
	10,
	0,
   },
   {
   	"Rate",
	Web_Rate_Id,
	12,
	0,
   },
    {
   	"Bssid",
	Web_Bssid_Id,
	18,
	12,
   },   
   {
   	"Channel",
  	Web_Channel_Id,
   	15,
   	0,
   },
   {
   	"Ch1",
	Web_ChannlListCh1_Id,
	19,
	0,
   },
    {
   	"Ch2",
	Web_ChannlListCh2_Id,
	19,
	0,
   },
   {
   	"Ch3",
	Web_ChannlListCh3_Id,
	19,
	0,
   },
   {
   	"Ch4",
	Web_ChannlListCh4_Id,
	19,
	0,
   },
    {
   	"Ch5",
	Web_ChannlListCh5_Id,
	19,
	0,
   },
    {
   	"Ch6",
	Web_ChannlListCh6_Id,
	19,
	0,
   },
   {
   	"Ch7",
	Web_ChannlListCh7_Id,
	19,
	0,
   },
    {
   	"Ch8",
	Web_ChannlListCh8_Id,
	19,
	0,
   },
    {
   	"Ch9",
	Web_ChannlListCh9_Id,
	19,
	0,
   },
    {
   	"Ch10",
	Web_ChannlListCh10_Id,
	20,
	0,
   },
    {
   	"Ch11",
	Web_ChannlListCh11_Id,
	20,
	0,
   },
    {
   	"Ch12",
	Web_ChannlListCh12_Id,
	20,
	0,
   },
    {
   	"Ch13",
	Web_ChannlListCh13_Id,
	20,
	0,
   },
     {
   	"Ch14",
	Web_ChannlListCh14_Id,
	20,
	0,
   }, 
   {
   	"WebPort",
	Web_WebPort_Id,
	20,
	2,
   },
   {
  	 "Baud",
  	 Web_Baud_Id,
   	12,
   	0,
   },
  {
  	"Check",
	 Web_Check_Id,
	 13,
	 0,
  },
  {
  	"DataSize",
  	Web_DataSize_Id,
  	16,
  	0,
  },
  {
  	"StopSize",
  	Web_StopSize_Id,
  	16,
  	0,
  },  
  {
      "EscChar",
	Web_EscChar_Id,
	20,
	2,
  },
  {
      "EscP",
	Web_EscP_Id,
	17,
	4,
  },
   {
      "EscDataP",
	Web_EscDataP_Id,
	21,
	3,
  },
    {
      "EscDataL",
	Web_EscDataL_Id,
	21,
	3,
  },
  {
  	"GPIO1",
	Web_GPIO1_Id,
	13,
	0,
  },
  {
  	"CmdMode",
	Web_CmdMode_Id,
	15,
	0,
  },  
  {
  	"BssidEable",
	Web_BssidEable_Id,
	41,
	0,
  },
  {
  	"PassWord",
	Web_PassWord_Id,
	21,
	6,
  },
  {
  	"TCP_TimeOut",
	 Web_TCP_TimeOut_id,
	 24,
	 5,
	 
  },
  {
  	"MacAdr",
	 Web_SysInfo_Macaddr,
	 19,
	 1,
  },
  {
  	"HardVer",
	 Web_SysInfo_HardVer,
	 20,
	 1,
  },
  {
  	"FirmVer",
	 Web_SysInfo_FirmVer,
	 20,
	 1,
  },
  {
  	"RelTime",
	 Web_SysInfo_RelTime,
	 20,
	 1,
  },
  {
  	"siteInfo",
	 Web_SiteInfo_Copyright,
	 34,
	 0,
  },
  {
  	"Try_times",
	Web_Try_times_id,
	22,
	3,
  },
  {
	 "roam",
 	 Web_roam_id,
  	10,
  	0,
  },
  {
  	"DnsName",
	Web_DnsName_Id,
	20,
	1,
  }, 
  {
  	"SsidBenable",
	Web_SsidBroadcast_Id,
	17,
	0,  
  },
  {
  	"autocreateadhoc",
	Web_Autocreateadhoc_Id,
	21,
	0,  
  },

  {
  	"powersave",
	Web_powersave_id,
	15,
	0,  
  },
  {
     "SsidSL",
     Web_scan_id,
    14,
    0,  
  },
  {
     "Encryweb",
     Web_WebEncry_id,
    35,
    0,  
  },	
  {
  	"AutoHiden",
	Web_AutoHidden_Id,
	22,
	1,
  }
#if 0
  ,

   NULL,
#endif
};

#if 0
char * Encry_Open[]={"<option value=\"0\" selected=\"selected\">Disable </option> <option value=\"1\" >WEP64</option> <option value=\"2\" >WEP128</option> <option value=\"3\">WPA-PSK(TKIP)</option> <option value=\"4\">WPA-PSK(CCMP)</option> <option value=\"5\">WPA2-PSK(TKIP)</option> <option value=\"6\">WPA2_PSK(CCMP)</option>\n", 
	                             "<option value=\"0\" >Disable </option> <option value=\"1\" selected=\"selected\">WEP64</option> <option value=\"2\" >WEP128</option> <option value=\"3\">WPA-PSK(TKIP)</option> <option value=\"4\">WPA-PSK(CCMP)</option> <option value=\"5\">WPA2-PSK(TKIP)</option> <option value=\"6\">WPA2_PSK(CCMP)</option>\n",    
	                             "<option value=\"0\" >Disable </option> <option value=\"1\" >WEP64</option> <option value=\"2\" selected=\"selected\">WEP128</option> <option value=\"3\">WPA-PSK(TKIP)</option> <option value=\"4\">WPA-PSK(CCMP)</option> <option value=\"5\">WPA2-PSK(TKIP)</option> <option value=\"6\">WPA2_PSK(CCMP)</option>\n",    
	                             "<option value=\"0\" >Disable </option> <option value=\"1\" >WEP64</option> <option value=\"2\" >WEP128</option> <option value=\"3\"selected=\"selected\">WPA-PSK(TKIP)</option> <option value=\"4\">WPA-PSK(CCMP)</option> <option value=\"5\">WPA2-PSK(TKIP)</option> <option value=\"6\">WPA2_PSK(CCMP)</option>\n",    
	                             "<option value=\"0\" >Disable </option> <option value=\"1\" >WEP64</option> <option value=\"2\" >WEP128</option> <option value=\"3\">WPA-PSK(TKIP)</option> <option value=\"4\" selected=\"selected\">WPA-PSK(CCMP)</option> <option value=\"5\">WPA2-PSK(TKIP)</option> <option value=\"6\">WPA2_PSK(CCMP)</option>\n",   
	                             "<option value=\"0\" >Disable </option> <option value=\"1\" >WEP64</option> <option value=\"2\" >WEP128</option> <option value=\"3\">WPA-PSK(TKIP)</option> <option value=\"4\">WPA-PSK(CCMP)</option> <option value=\"5\"selected=\"selected\">WPA2-PSK(TKIP)</option> <option value=\"6\">WPA2_PSK(CCMP)</option>\n",    
	                             "<option value=\"0\" >Disable </option> <option value=\"1\" >WEP64</option> <option value=\"2\" >WEP128</option> <option value=\"3\">WPA-PSK(TKIP)</option> <option value=\"4\">WPA-PSK(CCMP)</option> <option value=\"5\">WPA2-PSK(TKIP)</option> <option value=\"6\"selected=\"selected\">WPA2_PSK(CCMP)</option>\n"    
	                             };
#endif
//char * Encry_Open[]={"whet", "WEP64", "WEP128", "WPA-PSK(TKIP)", "are(CCMP)", "WPA2-PSK(TKIP)", "you(CCMP)"};

#if WEB_SERVER_RUSSIAN
extern int gCurHtmlFile;
char * Web_Mode[]={"<option value=\"0\" selected=\"selected\">Station (end-point)</option> <option value=\"1\">Ad hoc network</option> <option value=\"2\">Access Point</option>\n", 
					"<option value=\"0\" >Station (end-point)</option> <option value=\"1\" selected=\"selected\">Ad hoc network</option> <option value=\"2\">Access Point</option>\n",    
					"<option value=\"0\" >Station (end-point)</option> <option value=\"1\">Ad hoc network</option> <option value=\"2\" selected=\"selected\">Access Point</option>\n",
					"<option value=\"0\" selected=\"selected\">孰桢眚</option> <option value=\"1\">Ad hoc 皴蜩</option> <option value=\"2\">翌麝?漕耱箫?/option>\n",
					"<option value=\"0\" >孰桢眚</option> <option value=\"1\" selected=\"selected\">Ad hoc 皴蜩</option> <option value=\"2\">翌麝?漕耱箫?/option>\n",
					"<option value=\"0\" >孰桢眚</option> <option value=\"1\">Ad hoc 皴蜩</option> <option value=\"2\" selected=\"selected\">翌麝?漕耱箫?/option>\n"
};
#else
/*
char * Web_Mode[]={"<option value=\"0\" selected=\"selected\">Sta</option> <option value=\"1\">Adhoc</option> <option value=\"2\">AP</option>\n",
					"<option value=\"0\" >Sta</option> <option value=\"1\" selected=\"selected\">Adhoc</option> <option value=\"2\">AP</option>\n",
					"<option value=\"0\" >Sta</option> <option value=\"1\" >Adhoc</option> <option value=\"2\" selected=\"selected\">AP</option>\n"
					};
*/
#endif
/*
const char Web_null[] = "";
const char Web_selected[] = "selected=\"selected\"";
const char Web_checked[] = "checked=\"checked\"";
*/

#define BAUDRATE_BELOW  8
//u32 Web_Baud[BAUDRATE_BELOW]= {115200, 57600, 38400, 19200, 9600, 4800, 2400, 1200};
//char *Web_Baud[] = {"115200", "57600", "38400", "19200", "9600", "4800", "2400", "1200" };
/*
char *Web_Rate[] = {"1M", "2M", "5.5M", "11M", "6M", "9M", "12M", "18M", "24M", "36M", "48M", "54M", 
			"MCS0", "MCS1", "MCS2", "MCS3", "MCS4", "MCS5", "MCS6", "MCS7", "MCS8", "MCS9", 
			"MCS10", "MCS11", "MCS12", "MCS13", "MCS14", "MCS15", "MCS32"};
*/			
/*
char * Web_DataSize[]={"<option value=\"0\" selected=\"selected\">8</option> <option value=\"1\">7</option>\n",
                                        "<option value=\"0\" >8</option> <option value=\"1\" selected=\"selected\">7</option>\n",
	};
*/
// kevin modify 20131227 only 8 bit 
/*
char * Web_DataSize[]={"<option value=\"0\" selected=\"selected\">8</option>\n",
};

char * Auto_type[]={"onclick=\"auto()\" value=\"1\"  /> Auto Mode Enable </label>\n",
				      "onclick=\"auto()\" value=\"1\" checked=\"CHECKED\" />  Auto Mode Enable </label>\n"
	};
char * Protocol_type[]={"<option value=\"0\" selected=\"selected\">TCP</option> <option value=\"1\">UDP</option>\n",
				           "<option value=\"0\" >TCP</option> <option value=\"1\" selected=\"selected\">UDP</option>\n"
	};
char * Cs_type[]={"<option value=\"0\" selected=\"selected\">CLIENT</option> <option value=\"1\" >SERVER</option>\n",
				  "<option value=\"0\">CLIENT</option> <option value=\"1\" selected=\"selected\">SERVER</option>\n"};

char * able_type[]={"> <option value=\"0\" selected=\"selected\">Disable</option> <option value=\"1\">Enable</option>\n",
				  "> <option value=\"0\" >Disable</option> <option value=\"1\" selected=\"selected\">Enable</option>\n",
				  "disabled=\"disabled\"> <option value=\"0\"  selected=\"selected\">Disable</option> <option value=\"1\">Enable</option>\n"};

char * Key_Index[]={"checked=\"checked\" />\n",
					"/>\n"};
*/					
/*-----------------------------------------------------------------------------------
// Description:   Read one line text into buffer s, and return its length 
// Parameters:  file:
			   char buffer :buffer to store the text read from file 
//                     max_length: max length of one line  
*/
#define MAX_ID_BUFFER_LEN 15
#define BSS_INFO_SIZE 2048

static u8 scan_done = 0;
u8 gwebcfgmode = 0;


void scan_result_cb(void)
{
    scan_done = 1;
}

u16  Web_parse_line(char * id_char,u16 * after_id_len,char * idvalue,u16 * Value_Offset,u8 * Id_type)
{
    /*
	char idbuffer[MAX_ID_BUFFER_LEN];
	u8 j=0, mode, encrypt;
	ID_VALUE  *idtble;
	u8 tableid=0;
	struct tls_param_ip ip_param;
	short channellist;
	struct tls_param_key param_key;
	u8 auto_mode;
	struct tls_param_socket remote_socket_cfg;
	struct tls_param_bssid bssid;
	struct tls_param_uart uart_cfg;
	*/
	char idbuffer[MAX_ID_BUFFER_LEN];
	u8 j=0, mode;//, encrypt;
	ID_VALUE  *idtble;
	u8 tableid=0;
    struct tls_param_ip ip_param;
	struct tls_param_key param_key;
    int k;

	memset(idbuffer,0,MAX_ID_BUFFER_LEN);
	* Id_type=0;
	while(*(id_char+j)!='"')
	{j++;}
	memcpy(idbuffer,id_char,j);

    for (k = 0; k < sizeof(web_id_tbl) / sizeof(ID_VALUE); k++)
    {
        idtble = (ID_VALUE *)&web_id_tbl[k];
        if (strcmp(idbuffer,idtble->IdName) == 0)
		{
		  tableid=idtble->tableid;
		  break;
		}
    }

#if 0
	for (idtble = (ID_VALUE *)&web_id_tbl[0]; idtble->IdName; idtble++) 
	{
		if (strcmp(idbuffer,idtble->IdName) == 0)
		{
		  tableid=idtble->tableid;
		  break;
		}
	}
#endif

//	DBGPRINT("###kevin debug Web_parse_line = %s\n\r",idbuffer);
	tls_param_get(TLS_PARAM_ID_IP, &ip_param, FALSE);
	switch(tableid)
	{
	case Dhcp_Id:
        /*
		tls_param_get(TLS_PARAM_ID_WPROTOCOL, (void* )&mode, FALSE);
		  if(ip_param.dhcp_enable)
		  {
		  	sprintf(idvalue, "onclick=\"dhcp1()\" value=\"1\" checked=\"CHECKED\"  %s/>Auto IP Enable</label></td>\n", mode==IEEE80211_MODE_AP ? "disabled=\"disabled\" " : "");
		  }
		  else
		  {
		  	sprintf(idvalue, "onclick=\"dhcp1()\" value=\"1\"  %s/>Auto IP Enable</label></td>\n", mode==IEEE80211_MODE_AP ? "disabled=\"disabled\" " : "");
		  }
		  
		*after_id_len=idtble->Idlen;
		*Value_Offset=idtble->Value_Offset;
		* Id_type=0x01;	
		*/
		break;
	case Web_Ip_Id:
        /*
		tls_param_get(TLS_PARAM_ID_WPROTOCOL, (void* )&mode, FALSE);
		if(!ip_param.dhcp_enable || mode==IEEE80211_MODE_AP)
		{
	 	sprintf(idvalue, "%d.%d.%d.%d", \
					ip_param.ip[0], ip_param.ip[1],\
					ip_param.ip[2], ip_param.ip[3]);
		

		*after_id_len=idtble->Idlen;
		*Value_Offset=idtble->Value_Offset;
		}
		
		else
		{
	 	sprintf(idvalue, "%d.%d.%d.%d\" disabled=\"disabled\" ", \
					0, 0,\
					0, 0);
		

		*after_id_len=idtble->Idlen;
		*Value_Offset=idtble->Value_Offset+1;		
		}
		*/
		break;
	case Web_Sub_Id:
        /*
		tls_param_get(TLS_PARAM_ID_WPROTOCOL, (void* )&mode, FALSE);
		if(!ip_param.dhcp_enable || mode==IEEE80211_MODE_AP)
		{
	 	sprintf(idvalue, "%d.%d.%d.%d", \
					ip_param.netmask[0], ip_param.netmask[1],\
					ip_param.netmask[2], ip_param.netmask[3]);
		*after_id_len=idtble->Idlen;
		*Value_Offset=idtble->Value_Offset;
		}
		else
		{
	 	sprintf(idvalue, "%d.%d.%d.%d\" disabled=\"disabled\" ", \
					0,0,\
					0, 0);
		*after_id_len=idtble->Idlen;
		*Value_Offset=idtble->Value_Offset+1;
		}
		*/
		break;
	case DNS_Gate_Id:
        /*
		tls_param_get(TLS_PARAM_ID_WPROTOCOL, (void* )&mode, FALSE);
		if((!ip_param.dhcp_enable)&&(mode != IEEE80211_MODE_AP))
		{		
	 	sprintf(idvalue, "%d.%d.%d.%d", \
			ip_param.gateway[0], ip_param.gateway[1],\
			ip_param.gateway[2], ip_param.gateway[3]);

		*after_id_len=idtble->Idlen;
		*Value_Offset=idtble->Value_Offset;
		}
		else
		{
	 	sprintf(idvalue, "%d.%d.%d.%d\" disabled=\"disabled\" ",\
			0, 0,\
			0, 0);

		*after_id_len=idtble->Idlen;
		*Value_Offset=idtble->Value_Offset+1;		
		}
		*/

		break;
	case DNS_Dns_Id:
        /*
		tls_param_get(TLS_PARAM_ID_WPROTOCOL, (void* )&mode, FALSE);
		if((!ip_param.dhcp_enable)&&(mode != IEEE80211_MODE_AP))
		{		
	 	sprintf(idvalue, "%d.%d.%d.%d", \
			ip_param.dns1[0], ip_param.dns1[1],\
			ip_param.dns1[2], ip_param.dns1[3]);

		*after_id_len=idtble->Idlen;
		*Value_Offset=idtble->Value_Offset;
		}
		else
		{
	 	sprintf(idvalue, "%d.%d.%d.%d\" disabled=\"disabled\" ",\
			0, 0,\
			0, 0);

		*after_id_len=idtble->Idlen;
		*Value_Offset=idtble->Value_Offset+1;		
		}
		*/
		break;
	case Web_DnsName_Id:
        /*
		{
		u8 dnsname[32];
		tls_param_get(TLS_PARAM_ID_WPROTOCOL, (void* )&mode, FALSE);
		tls_param_get(TLS_PARAM_ID_DNSNAME, dnsname, FALSE);
		if(mode == IEEE80211_MODE_AP)
		{		
			{
				sprintf(idvalue, "%s",	(char *)dnsname);
			}
			*after_id_len=idtble->Idlen;
			*Value_Offset=idtble->Value_Offset;
		}
		else
		{
			sprintf(idvalue, "%s\" disabled=\"disabled\" ", (char *)dnsname);
			*after_id_len=idtble->Idlen;
			*Value_Offset=idtble->Value_Offset+1;	
		}
		}
		*/
		break;

        case Web_Encry_Id:
        
	    {
			if (gwebcfgmode == 0)
			{
	    		char str[100];
	            struct tls_scan_bss_t *scan_res = NULL;
	            struct tls_bss_info_t *bss_info;

	            u8 *bssBuff = NULL;
	            bssBuff = tls_mem_alloc(BSS_INFO_SIZE);
	            memset(bssBuff, 0, sizeof(BSS_INFO_SIZE));
	                
	            tls_wifi_scan_result_cb_register(scan_result_cb);
	        	while (WM_SUCCESS !=tls_wifi_scan()) {
	        		tls_os_time_delay(HZ/5);
	        	}
	            while( scan_done == 0 ) {
	                tls_os_time_delay(HZ/5);
	            }
	            scan_done = 0;
	            tls_wifi_get_scan_rslt(bssBuff, BSS_INFO_SIZE);
	    
	            scan_res = (struct tls_scan_bss_t *)bssBuff;
	            bss_info = scan_res->bss;
	            DEBUG_PRINT("scan: %d\r\n", scan_res->count);
	            strcpy(idvalue, "arentNode.previousSibling.value=this.value\">\n");
	            sprintf(str, "<option value=\"\"></option>\n");
	            strcat(idvalue, str);
	            for(u8 i=0; i<scan_res->count && i<40; i++)
	            {
	                if( bss_info->ssid != NULL && bss_info->ssid_len != 0 )
					{
	                    *(bss_info->ssid + bss_info->ssid_len) = '\0';
	                    sprintf(str, "<option value=\"%s\">%s</option>\n", bss_info->ssid,  bss_info->ssid);
	        			strcat(idvalue, str);
			            bss_info->ssid[bss_info->ssid_len] = '\0';
	                    DEBUG_PRINT("legal: %s\r\n", bss_info->ssid);
	                }
	                else
	                {
		           		bss_info->ssid[bss_info->ssid_len] = '\0';
	                    DEBUG_PRINT("illegal: %d, %s\r\n", bss_info->ssid_len, bss_info->ssid);
	                }
	                bss_info ++;				
	            }

	            if( bssBuff != NULL ) {
	                tls_mem_free(bssBuff);
	                bssBuff = NULL;
	            }
			}
            
    		*after_id_len = idtble->Idlen-4;
    		*Value_Offset = idtble->Value_Offset;		
    		*Id_type = 0x01;		
	    }
	    
		break;
        
	case Web_Ssid_Id:
		{            
    		struct tls_param_ssid ssid;
    		tls_param_get(TLS_PARAM_ID_SSID, (void *)&ssid, FALSE);
    	 	 memcpy(idvalue, ssid.ssid, ssid.ssid_len);

    		*after_id_len=idtble->Idlen;
    		*Value_Offset=idtble->Value_Offset;
		}
		break;		

        /*
	case Web_Encry_Id:
        
	    {
		int i = 0;
		char str[100];
		tls_param_get(TLS_PARAM_ID_WPROTOCOL, (void* )&mode, FALSE);
		tls_param_get(TLS_PARAM_ID_ENCRY, (void *)&encrypt, FALSE);
		if(mode == IEEE80211_MODE_INFRA)
		{
			//strcpy(idvalue, " disabled=\"disabled\">");
			strcpy(idvalue, " >");
			for(i=0;i<6;i++){
				sprintf(str, "<option value=\"%d\" %s>%s</option>", i, \
					(i == 0)? "selected=\"selected\"" : "", Encry_Open[i]);
				strcat(idvalue, str);
			}
			*after_id_len = idtble->Idlen-4;
			*Value_Offset = idtble->Value_Offset;		
			*Id_type = 0x01;
		}
		else	 if(mode == IEEE80211_MODE_AP)
		{		
			for(i=0;i<7;i++){
				sprintf(str, "<option value=\"%d\" %s>%s</option>", i, \
					(i == encrypt)? "selected=\"selected\"" : "", Encry_Open[i]);
				strcat(idvalue, str);
			}
			*after_id_len=idtble->Idlen;
			*Value_Offset = idtble->Value_Offset;		
			*Id_type = 0x01;
		}
		else				
		{		
			for(i=0;i<3;i++){
				sprintf(str, "<option value=\"%d\" %s>%s</option>", i, \
					(i == encrypt)? "selected=\"selected\"" : "", Encry_Open[i]);
				strcat(idvalue, str);
			}
			*after_id_len=idtble->Idlen;
			*Value_Offset = idtble->Value_Offset;		
			*Id_type = 0x01;
		}
	    }
	    
		break;
        */
	case Web_Key_Id:
		tls_param_get(TLS_PARAM_ID_WPROTOCOL, (void* )&mode, FALSE);
		tls_param_get(TLS_PARAM_ID_KEY, (void *)&param_key, FALSE);
//		tls_param_get(TLS_PARAM_ID_ENCRY, (void *)&encrypt, FALSE);
		if((mode & IEEE80211_MODE_INFRA) ||(IEEE80211_MODE_AP & mode))// (encrypt != ENCRYPT_TYPE_OPEN_SYS))
		{
			DEBUG_PRINT("key:%s, keyLen:%d\n", param_key.psk, param_key.key_length);
			int  j = 0;
			for (int i = 0; i < param_key.key_length; i++)
			{
				if (param_key.psk[i] == '\"')
				{
					/*&#34;*/
					*(idvalue + j) = '&';				
					j++;
					*(idvalue + j) = '#';				
					j++;
					*(idvalue + j) = '3';				
					j++;
					*(idvalue + j) = '4';
					j++;
					*(idvalue + j) = ';';				
					j++;
				}
				else if (param_key.psk[i] == '\'')
				{
					/*&#34;*/
					*(idvalue + j) = '&';				
					j++;
					*(idvalue + j) = '#';				
					j++;
					*(idvalue + j) = '3';				
					j++;
					*(idvalue + j) = '9';
					j++;
					*(idvalue + j) = ';';				
					j++;	
				}
				else
				{
					*(idvalue + j) = param_key.psk[i];
					j++;
				}
			}
			
			//sprintf(idvalue, "%s",  param_key.psk);	
			//idvalue[param_key.key_length] = '\0';
			idvalue[j] = '\0';
			*after_id_len=idtble->Idlen;
			*Value_Offset=idtble->Value_Offset;
		}
		else
		{
			//sprintf(idvalue, "%d\" disabled=\"disabled\" ",  0);
			sprintf(idvalue, "\" disabled=\"disabled\" ");
			*after_id_len=idtble->Idlen;
			*Value_Offset=idtble->Value_Offset+1;
		}
		break;
	case KeyType_Id:
        /*
		tls_param_get(TLS_PARAM_ID_KEY, (void *)&param_key, FALSE);
		tls_param_get(TLS_PARAM_ID_WPROTOCOL, (void* )&mode, FALSE);
		tls_param_get(TLS_PARAM_ID_ENCRY, (void *)&encrypt, FALSE);
		if((mode == IEEE80211_MODE_INFRA) 
			|| (encrypt == ENCRYPT_TYPE_OPEN_SYS))
		{
			if(param_key.key_format ==1)///Sever
			{
				sprintf(idvalue," disabled=\"disabled\"> <option value=\"0\">HEX</option> <option value=\"1\" selected=\"selected\">ASCII</option>\n");	
			}
			else    //////Client
			{
				sprintf(idvalue," disabled=\"disabled\"> <option value=\"0\" selected=\"selected\">HEX</option> <option value=\"1\">ASCII</option>\n");	
			}
			*after_id_len=idtble->Idlen-2;
			*Value_Offset=idtble->Value_Offset;
			* Id_type=0x01;	
		}
		else
		{
			if(param_key.key_format ==1)
			{
				strcpy(idvalue,"<option value=\"0\">HEX</option> <option value=\"1\" selected=\"selected\">ASCII</option>\n");
			}
			else
			{
				strcpy(idvalue,"<option value=\"0\" selected=\"selected\">HEX</option> <option value=\"1\">ASCII</option>\n");
			}
			*after_id_len=idtble->Idlen;
			*Value_Offset=idtble->Value_Offset;
			* Id_type=0x01;
		}
		*/
		break;
	case Web_Auto_Id:
        /*
		tls_param_get(TLS_PARAM_ID_AUTOMODE, (void *)&auto_mode, FALSE);
#if 1
		if(auto_mode)
		{
		  	strcpy(idvalue,Auto_type[1]);	
		}
		else
		{
			strcpy(idvalue,Auto_type[0]);	
		}
#else
		{
			sprintf(idvalue, "onclick=\"auto()\" value=\"%d\" %s />   远模式</label></td>\n",\
				(!auto_mode), \
				(!auto_mode) ? Web_null : Web_checked); 
		}
#endif
		
		*after_id_len=idtble->Idlen;
		*Value_Offset=idtble->Value_Offset;
		* Id_type=0x01;	
		*/
		break;
	case Web_AutoHidden_Id:
        /*
		tls_param_get(TLS_PARAM_ID_AUTOMODE, (void *)&auto_mode, FALSE);
		if(auto_mode)
		{
		  	strcpy(idvalue, "1");	
		}
		else
		{
			strcpy(idvalue, "0");	
		}
		
		*after_id_len=idtble->Idlen;
		*Value_Offset=idtble->Value_Offset;
		*/
		//* Id_type=0x01;		
		break;
	case Web_Protocol_Id:
        /*
    	tls_param_get(TLS_PARAM_ID_DEFSOCKET, &remote_socket_cfg, FALSE);
		tls_param_get(TLS_PARAM_ID_AUTOMODE, (void *)&auto_mode, FALSE);
		if(auto_mode==1)
		{
			if(remote_socket_cfg.protocol)
			{
				sprintf(idvalue,"%s",Protocol_type[1]);	
			}
			else  ////TCP
			{
				sprintf(idvalue,"%s",Protocol_type[0]);
			}
			*after_id_len=idtble->Idlen;
			*Value_Offset=idtble->Value_Offset;
			* Id_type=0x01;	
		}
		else
		{
			if(remote_socket_cfg.protocol)
			{
				sprintf(idvalue,"disabled=\"disabled\" %s",Protocol_type[1]);	
			}
			else  ////TCP
			{
				sprintf(idvalue,"disabled=\"disabled\" %s",Protocol_type[0]);
			}
			*after_id_len=idtble->Idlen-2;
			*Value_Offset=idtble->Value_Offset;
			* Id_type=0x01;				
		}
		*/
		break;
	case Web_Cs_Id:
        /*
    	tls_param_get(TLS_PARAM_ID_DEFSOCKET, &remote_socket_cfg, FALSE);
		tls_param_get(TLS_PARAM_ID_AUTOMODE, (void *)&auto_mode, FALSE);
		if(auto_mode==1)
		{		
			if(remote_socket_cfg.client_or_server)///Sever
			{
				strcpy(idvalue,Cs_type[1]);	
		
			}
			else    //////Client
			{
				strcpy(idvalue,Cs_type[0]);	
			}
			*after_id_len=idtble->Idlen;
			*Value_Offset=idtble->Value_Offset;
			* Id_type=0x01;	
		}
		else
		{
			if(remote_socket_cfg.client_or_server)///Sever
			{
				sprintf(idvalue,"disabled=\"disabled\" %s",Cs_type[1]);	
		
			}
			else    //////Client
			{
				sprintf(idvalue,"disabled=\"disabled\" %s",Cs_type[0]);	
			}
			*after_id_len=idtble->Idlen-2;
			*Value_Offset=idtble->Value_Offset;
			* Id_type=0x01;			
		}
		*/
		break;
	case Web_Domain_Id:
        /*
    	tls_param_get(TLS_PARAM_ID_DEFSOCKET, &remote_socket_cfg, FALSE);
		tls_param_get(TLS_PARAM_ID_AUTOMODE, (void *)&auto_mode, FALSE);
		if(auto_mode==1)
		{		
			if(remote_socket_cfg.client_or_server && remote_socket_cfg.protocol == 0)
			{
				sprintf(idvalue, "%s \" disabled=\"disabled\"", "0.0.0.0");
				*after_id_len=idtble->Idlen;
				*Value_Offset=idtble->Value_Offset+1;	
				break;
			}
			else
			{
				sprintf(idvalue, "%s",  (char *)remote_socket_cfg.host);
			}
			*after_id_len=idtble->Idlen;
			*Value_Offset=idtble->Value_Offset;
		}
		else
		{
			sprintf(idvalue, "%s\" disabled=\"disabled\" ",  "0.0.0.0");
			*after_id_len=idtble->Idlen;
			*Value_Offset=idtble->Value_Offset+1;	
		}
		*/

		break;		
	case Web_Port_Id:
        /*
    	tls_param_get(TLS_PARAM_ID_DEFSOCKET, &remote_socket_cfg, FALSE);
		tls_param_get(TLS_PARAM_ID_AUTOMODE, (void *)&auto_mode, FALSE);
		if(auto_mode==1)
		{
			sprintf(idvalue, "%d",  remote_socket_cfg.port_num);
			*after_id_len=idtble->Idlen;
			*Value_Offset=idtble->Value_Offset;
		}
		else
		{
			sprintf(idvalue, "%d\" disabled=\"disabled\" ",  0);
			*after_id_len=idtble->Idlen;
			*Value_Offset=idtble->Value_Offset+1;
		}
		*/

		break;	
	case Wep_KeyIndex_Id:
        /*
		tls_param_get(TLS_PARAM_ID_WPROTOCOL, (void* )&mode, FALSE);
		tls_param_get(TLS_PARAM_ID_KEY, (void *)&param_key, FALSE);
		tls_param_get(TLS_PARAM_ID_ENCRY, (void *)&encrypt, FALSE);
		if((mode != IEEE80211_MODE_INFRA)  && (encrypt==ENCRYPT_TYPE_WEP_64_BITS||encrypt==ENCRYPT_TYPE_WEP_128_BITS))
		{
			if(param_key.key_index==1)///Sever
			{
				sprintf(idvalue, "%s","checked=\"CHECKED\" />\n");
			}
			else
			{
				sprintf(idvalue, "%s","/>\n");	
			}
		}
		else
		{
				sprintf(idvalue, "%s","disabled=\"disabled\" />\n");	
	
		}
		*after_id_len=idtble->Idlen;
		*Value_Offset=idtble->Value_Offset;
		* Id_type=0x01;	
		*/
		break;
	case Wep_KeyIndex2_Id:
        /*
		tls_param_get(TLS_PARAM_ID_WPROTOCOL, (void* )&mode, FALSE);
		tls_param_get(TLS_PARAM_ID_KEY, (void *)&param_key, FALSE);
		tls_param_get(TLS_PARAM_ID_ENCRY, (void *)&encrypt, FALSE);
		if((mode != IEEE80211_MODE_INFRA)  && (encrypt==ENCRYPT_TYPE_WEP_64_BITS||encrypt==ENCRYPT_TYPE_WEP_128_BITS))
		{
		if(param_key.key_index==2)//
		{
			sprintf(idvalue, "%s","checked=\"CHECKED\" />\n");	
		
		}
		else
		{
			sprintf(idvalue, "%s","/>\n");	
		}
		}
		else
		{
			sprintf(idvalue, "%s","disabled=\"disabled\" />\n");	
	
		}		
		*after_id_len=idtble->Idlen;
		*Value_Offset=idtble->Value_Offset;
		* Id_type=0x01;	
		*/
		break;	
	case Wep_KeyIndex3_Id:
        /*
		tls_param_get(TLS_PARAM_ID_WPROTOCOL, (void* )&mode, FALSE);
		tls_param_get(TLS_PARAM_ID_KEY, (void *)&param_key, FALSE);
		tls_param_get(TLS_PARAM_ID_ENCRY, (void *)&encrypt, FALSE);
		if((mode != IEEE80211_MODE_INFRA)  && (encrypt==ENCRYPT_TYPE_WEP_64_BITS||encrypt==ENCRYPT_TYPE_WEP_128_BITS))
		{		
		if(param_key.key_index==3)///
		{
			sprintf(idvalue, "%s","checked=\"CHECKED\" />\n");	
		
		}
		else
		{
			sprintf(idvalue, "%s","/>\n");	
		}
		}
		else
		{
			sprintf(idvalue, "%s","disabled=\"disabled\" />\n");	
	
		}			
		*after_id_len=idtble->Idlen;
		*Value_Offset=idtble->Value_Offset;
		* Id_type=0x01;	
		*/
		break;	
	case Wep_KeyIndex4_Id:
        /*
		tls_param_get(TLS_PARAM_ID_WPROTOCOL, (void* )&mode, FALSE);
		tls_param_get(TLS_PARAM_ID_KEY, (void *)&param_key, FALSE);
		tls_param_get(TLS_PARAM_ID_ENCRY, (void *)&encrypt, FALSE);
		if((mode != IEEE80211_MODE_INFRA)  && (encrypt==ENCRYPT_TYPE_WEP_64_BITS||encrypt==ENCRYPT_TYPE_WEP_128_BITS))
		{		
		if(param_key.key_index==4)///
		{
			sprintf(idvalue, "%s","checked=\"CHECKED\" />\n");	
		
		}
		else
		{
			sprintf(idvalue, "%s","/>\n");	
		}
		}
		else
		{
			sprintf(idvalue, "%s","disabled=\"disabled\" />\n");	
	
		}			
		*after_id_len=idtble->Idlen;
		*Value_Offset=idtble->Value_Offset;
		* Id_type=0x01;	
		*/
		break;		
	case Web_Mode_Id:
        /*
		tls_param_get(TLS_PARAM_ID_WPROTOCOL, (void* )&mode, FALSE);
#if WEB_SERVER_RUSSIAN
		if(gCurHtmlFile)
		{
			strcpy(idvalue,Web_Mode[mode + 3]); // get russian str
		}
		else
		{
			strcpy(idvalue,Web_Mode[mode]);
		}
#else
		strcpy(idvalue,Web_Mode[mode]);
#endif
		*after_id_len=idtble->Idlen;
		*Value_Offset=idtble->Value_Offset;
		* Id_type=0x01;	
		*/
		break;
	case Web_Bg_Id:
        /*
		{
		struct tls_param_bgr wbgr;
		tls_param_get(TLS_PARAM_ID_WBGR, (void *)&wbgr, FALSE);
		if(wbgr.bg==0)/////bg
		{
			 strcpy(idvalue,"<option value=\"0\" selected=\"selected\">B/G</option> <option value=\"1\">B</option><option value=\"2\">B/G/N</option>\n");
		}
		else if(wbgr.bg==2) //bgn
		{
			strcpy(idvalue,"<option value=\"0\" >B/G</option> <option value=\"1\">B</option><option value=\"2\" selected=\"selected\">B/G/N</option> \n");		
		}
		else  ////b
		{
			strcpy(idvalue,"<option value=\"0\" >B/G</option> <option value=\"1\" selected=\"selected\">B</option><option value=\"2\">B/G/N</option>\n");	
		}
		*after_id_len=idtble->Idlen;
		*Value_Offset=idtble->Value_Offset;
		* Id_type=0x01;			
		}
		*/
		break;
	case Web_Rate_Id:
        /*
#if 0
		strcpy(idvalue,Web_Rate[wbgr.max_rate]);
#else
		{
			int i;
			char str[] = "<option value=\"11\" selected=\"selected\">54M</option> ";
			struct tls_param_bgr wbgr;
			tls_param_get(TLS_PARAM_ID_WBGR, (void *)&wbgr, FALSE);
			if(wbgr.bg ==0)/////bg
			{
				for(i=0;i<12;i++){
					sprintf(str, "<option value=\"%d\" %s>%s</option>", i, \
						(i == wbgr.max_rate)? "selected=\"selected\"" : "", Web_Rate[i]);
					strcat(idvalue, str);
				}
			}
			else if(wbgr.bg ==2) //bgn
			{
				for(i=0;i<29;i++){
					sprintf(str, "<option value=\"%d\" %s>%s</option>", i, \
						(i == wbgr.max_rate)? "selected=\"selected\"" : "", Web_Rate[i]);
					strcat(idvalue, str);
				}			
			}
			else
			{
				for(i=0;i<4;i++){
					sprintf(str, "<option value=\"%d\" %s>%s</option>", i, \
						(i == wbgr.max_rate)? "selected=\"selected\"" : "", Web_Rate[i]);
					strcat(idvalue, str);
				}			
			}
			strcat(idvalue, "\n");
		}
#endif		
		*after_id_len=idtble->Idlen;
		*Value_Offset=idtble->Value_Offset;
		* Id_type=0x01;
		*/
		break;
	case Web_Bssid_Id:
        /*
		{
			tls_param_get(TLS_PARAM_ID_BSSID, (void *)&bssid, FALSE);
			tls_param_get(TLS_PARAM_ID_WPROTOCOL, (void* )&mode, FALSE);
			u8 Bssid[6];		
			if(mode==0)
			{
	    			if(bssid.bssid_enable==1)
	    			{
	    				memcpy(Bssid, bssid.bssid, 6);
	    		 		sprintf(idvalue, "%02X%02X%02X%02X%02X%02X", Bssid[0],Bssid[1],Bssid[2], Bssid[3],Bssid[4], Bssid[5]);						
	    				*after_id_len=idtble->Idlen;
	    				*Value_Offset=idtble->Value_Offset;
	    			}
	    			else
	    			{
	    
	    				memset(Bssid, 0, 6);		
	    		 		sprintf(idvalue, "%02X%02X%02X%02X%02X%02X \" disabled=\"disabled\" ", Bssid[0],Bssid[1],Bssid[2], Bssid[3],Bssid[4], Bssid[5]);		
	    				*after_id_len=idtble->Idlen;
	    				*Value_Offset=idtble->Value_Offset+1;			
	    			}
			}
			else
			{
				memset(Bssid, 0, 6);		
				sprintf(idvalue, "%02X%02X%02X%02X%02X%02X\" disabled=\"disabled\" ", Bssid[0],Bssid[1],Bssid[2], Bssid[3],Bssid[4], Bssid[5]);		
				*after_id_len=idtble->Idlen;
				*Value_Offset=idtble->Value_Offset+1;							
			}
		}
		*/
		break;		
	case Web_Channel_Id:
        /*
		{
		u8 channel;
		u8 channel_en;
		tls_param_get(TLS_PARAM_ID_CHANNEL, (void *)&channel, FALSE);
		tls_param_get(TLS_PARAM_ID_CHANNEL_EN, (void *)&channel_en, FALSE);
#if 0
		strcpy(idvalue,Web_Channel[channel]);
#else
		{
			int i;
			char str[] = "<option value=\"0\" selected=\"selected\">Auto </option>";

			if (!channel_en){
				strcpy(idvalue, str);
			}
			else{
				strcpy(idvalue, "<option value=\"0\" >Auto </option>");
			}
			
			for(i=1;i<=14;i++){
				sprintf(str, "<option value=\"%d\" %s>%d</option>", i, \
					((channel_en)&&(i == channel))? "selected=\"selected\"" : "", i);
				strcat(idvalue, str);
			}

			strcat(idvalue, "\n");
		}
#endif
		*after_id_len=idtble->Idlen;
		*Value_Offset=idtble->Value_Offset;
		* Id_type=0x01;		
		}
		*/
		break;
	case Web_ChannlListCh1_Id:
        /*
		   tls_param_get(TLS_PARAM_ID_CHANNEL_LIST, (void *)&channellist, FALSE);
		   if(channellist&(0x01<<0))
		   {
			strcpy(idvalue,"checked=\"CHECKED\" />\n");		   	
		   }
		   else
		   {
			 strcpy(idvalue,"/>\n");		   
		   }
		  *after_id_len=idtble->Idlen;
		  *Value_Offset=idtble->Value_Offset;
		  * Id_type=0x01;	
		  */
		  break;
	case Web_ChannlListCh2_Id:
        /*
		   tls_param_get(TLS_PARAM_ID_CHANNEL_LIST, (void *)&channellist, FALSE);
		   if(channellist&(0x01<<1))
		   {
			strcpy(idvalue,"checked=\"CHECKED\" />\n");		   	
		   }
		   else
		   {
			 strcpy(idvalue,"/>\n");		   
		   }
		  *after_id_len=idtble->Idlen;
		  *Value_Offset=idtble->Value_Offset;
		  * Id_type=0x01;
		  */
		  break;
	case Web_ChannlListCh3_Id:
        /*
		   tls_param_get(TLS_PARAM_ID_CHANNEL_LIST, (void *)&channellist, FALSE);
		   if(channellist&(0x01<<2))
		   {
			strcpy(idvalue,"checked=\"CHECKED\" />\n");		   	
		   }
		   else
		   {
			 strcpy(idvalue,"/>\n");		   
		   }
		  *after_id_len=idtble->Idlen;
		  *Value_Offset=idtble->Value_Offset;
		  * Id_type=0x01;
		  */
		  break;	
	case Web_ChannlListCh4_Id:
        /*
		   tls_param_get(TLS_PARAM_ID_CHANNEL_LIST, (void *)&channellist, FALSE);
		   if(channellist&(0x01<<3))
		   {
			strcpy(idvalue,"checked=\"CHECKED\" />\n");		   	
		   }
		   else
		   {
			 strcpy(idvalue,"/>\n");		   
		   }
		  *after_id_len=idtble->Idlen;
		  *Value_Offset=idtble->Value_Offset;
		  * Id_type=0x01;
		  */
		  break;	
	case Web_ChannlListCh5_Id:
        /*
		   tls_param_get(TLS_PARAM_ID_CHANNEL_LIST, (void *)&channellist, FALSE);
		   if(channellist&(0x01<<4))
		   {
			strcpy(idvalue,"checked=\"CHECKED\" />\n");		   	
		   }
		   else
		   {
			 strcpy(idvalue,"/>\n");		   
		   }
		  *after_id_len=idtble->Idlen;
		  *Value_Offset=idtble->Value_Offset;
		  * Id_type=0x01;
		  */
		  break;	
	case Web_ChannlListCh6_Id:
        /*
		   tls_param_get(TLS_PARAM_ID_CHANNEL_LIST, (void *)&channellist, FALSE);
		   if(channellist&(0x01<<5))
		   {
			strcpy(idvalue,"checked=\"CHECKED\" />\n");		   	
		   }
		   else
		   {
			 strcpy(idvalue,"/>\n");		   
		   }
		  *after_id_len=idtble->Idlen;
		  *Value_Offset=idtble->Value_Offset;
		  * Id_type=0x01;
		  */
		  break;	
	case Web_ChannlListCh7_Id:
        /*
		   tls_param_get(TLS_PARAM_ID_CHANNEL_LIST, (void *)&channellist, FALSE);
		   if(channellist&(0x01<<6))
		   {
			strcpy(idvalue,"checked=\"CHECKED\" />\n");		   	
		   }
		   else
		   {
			 strcpy(idvalue,"/>\n");		   
		   }
		  *after_id_len=idtble->Idlen;
		  *Value_Offset=idtble->Value_Offset;
		  * Id_type=0x01;
		  */
		  break;	
	case Web_ChannlListCh8_Id:
        /*
		   tls_param_get(TLS_PARAM_ID_CHANNEL_LIST, (void *)&channellist, FALSE);
		   if(channellist&(0x01<<7))
		   {
			strcpy(idvalue,"checked=\"CHECKED\" />\n");		   	
		   }
		   else
		   {
			 strcpy(idvalue,"/>\n");		   
		   }
		  *after_id_len=idtble->Idlen;
		  *Value_Offset=idtble->Value_Offset;
		  * Id_type=0x01;
		  */
		  break;	
	case Web_ChannlListCh9_Id:
        /*
		   tls_param_get(TLS_PARAM_ID_CHANNEL_LIST, (void *)&channellist, FALSE);
		   if(channellist&(0x01<<8))
		   {
			strcpy(idvalue,"checked=\"CHECKED\" />\n");		   	
		   }
		   else
		   {
			 strcpy(idvalue,"/>\n");		   
		   }
		  *after_id_len=idtble->Idlen;
		  *Value_Offset=idtble->Value_Offset;
		  * Id_type=0x01;
		  */
		  break;	
	case Web_ChannlListCh10_Id:
        /*
		   tls_param_get(TLS_PARAM_ID_CHANNEL_LIST, (void *)&channellist, FALSE);
		   if(channellist&(0x01<<9))
		   {
			strcpy(idvalue,"checked=\"CHECKED\" />\n");		   	
		   }
		   else
		   {
			 strcpy(idvalue,"/>\n");		   
		   }
		  *after_id_len=idtble->Idlen;
		  *Value_Offset=idtble->Value_Offset;
		  * Id_type=0x01;
		  */
		  break;	
	case Web_ChannlListCh11_Id:
        /*
		   tls_param_get(TLS_PARAM_ID_CHANNEL_LIST, (void *)&channellist, FALSE);
		   if(channellist&(0x01<<10))
		   {
			strcpy(idvalue,"checked=\"CHECKED\" />\n");		   	
		   }
		   else
		   {
			 strcpy(idvalue,"/>\n");		   
		   }
		  *after_id_len=idtble->Idlen;
		  *Value_Offset=idtble->Value_Offset;
		  * Id_type=0x01;
		  */
		  break;	
	case Web_ChannlListCh12_Id:
        /*
		   tls_param_get(TLS_PARAM_ID_CHANNEL_LIST, (void *)&channellist, FALSE);
		   if(channellist&(0x01<<11))
		   {
			strcpy(idvalue,"checked=\"CHECKED\" />\n");		   	
		   }
		   else
		   {
			 strcpy(idvalue,"/>\n");		   
		   }
		  *after_id_len=idtble->Idlen;
		  *Value_Offset=idtble->Value_Offset;
		  * Id_type=0x01;
		  */
		  break;	
	case Web_ChannlListCh13_Id:
        /*
		   tls_param_get(TLS_PARAM_ID_CHANNEL_LIST, (void *)&channellist, FALSE);
		   if(channellist&(0x01<<12))
		   {
			strcpy(idvalue,"checked=\"CHECKED\" />\n");		   	
		   }
		   else
		   {
			 strcpy(idvalue,"/>\n");		   
		   }
		  *after_id_len=idtble->Idlen;
		  *Value_Offset=idtble->Value_Offset;
		  * Id_type=0x01;
		  */
		  break;	
	case Web_ChannlListCh14_Id:
        /*
		   tls_param_get(TLS_PARAM_ID_CHANNEL_LIST, (void *)&channellist, FALSE);
		   if(channellist&(0x01<<13))
		   {
			strcpy(idvalue,"checked=\"CHECKED\" />\n");		   	
		   }
		   else
		   {
			 strcpy(idvalue,"/>\n");		   
		   }
		  *after_id_len=idtble->Idlen;
		  *Value_Offset=idtble->Value_Offset;
		  * Id_type=0x01;
		  */
		  break;		
	case Web_WebPort_Id:
        /*
		{
		struct tls_webs_cfg webcfg;
		tls_param_get(TLS_PARAM_ID_WEBS_CONFIG, (void *)&webcfg, FALSE);
		if(1<webcfg.PortNum<10000)
			sprintf(idvalue, "%d",  webcfg.PortNum);
		*after_id_len=idtble->Idlen;
		*Value_Offset=idtble->Value_Offset;
		}
		*/
		break;
	case Web_Baud_Id:  /////BAUDRATE_115200
	/*
		tls_param_get(TLS_PARAM_ID_UART, (void *)&uart_cfg, FALSE);
#if 0		
		strcpy(idvalue,Web_Baud[pSysVal->CfgPara.UartCfg.BaudRate]);
#else
		{
			int i;
			char str[] = "<option value=\"0\" selected=\"selected\">115200</option> ";

			for(i=0;i<BAUDRATE_BELOW;i++)
			{
				sprintf(str, "<option value=\"%d\" %s>%d</option>", i, \
					(Web_Baud[i] == uart_cfg.baudrate)? "selected=\"selected\"" : "", \
					Web_Baud[i]);
				strcat(idvalue, str);
			}
			strcat(idvalue, "\n");
 		}
#endif
		*after_id_len=idtble->Idlen;
		*Value_Offset=idtble->Value_Offset;
		* Id_type=0x01;
		*/
		break;
	case Web_Check_Id:
        /*
		tls_param_get(TLS_PARAM_ID_UART, (void *)&uart_cfg, FALSE);
		if(uart_cfg.parity==0)
		{
			strcpy(idvalue,"<option value=\"0\" selected=\"selected\">None </option> <option value=\"1\">Odd </option> <option value=\"2\">Even </option>\n");
		}
		else if(uart_cfg.parity==1)
		{
			strcpy(idvalue,"<option value=\"0\" >None </option> <option value=\"1\" selected=\"selected\">Odd </option> <option value=\"2\">Even </option>\n");
		}
		else
		{
			strcpy(idvalue,"<option value=\"0\" >None </option> <option value=\"1\">Odd </option> <option value=\"2\" selected=\"selected\">Even </option>\n");		
		}
		*after_id_len=idtble->Idlen;
		*Value_Offset=idtble->Value_Offset;
		* Id_type=0x01;
		*/
		break;
	case Web_StopSize_Id:
        /*
		tls_param_get(TLS_PARAM_ID_UART, (void *)&uart_cfg, FALSE);
		if(uart_cfg.stop_bits==0)
		{
			strcpy(idvalue,"<option value=\"0\" selected=\"selected\">1</option> <option value=\"1\">2</option>\n");		
		}
		else if(uart_cfg.stop_bits==1)
		{
			strcpy(idvalue,"<option value=\"0\" >1</option> <option value=\"1\" selected=\"selected\">2</option>\n");		
		}
		else
		{
		}
		*after_id_len=idtble->Idlen;
		*Value_Offset=idtble->Value_Offset;
		* Id_type=0x01;
		*/
		break;
	case Web_DataSize_Id:
        /*
		//strcpy(idvalue,Web_DataSize[pSysVal->CfgPara.UserIntfCfg.Uart.DataBits]);
		// kevin modify 20131227 only 8 bit 
		strcpy(idvalue,Web_DataSize[0]);
		*after_id_len=idtble->Idlen;
		*Value_Offset=idtble->Value_Offset;
		* Id_type=0x01;
		*/
		break;
	case Web_EscChar_Id:
        /*
		{
		u8 escapechar;
		tls_param_get(TLS_PARAM_ID_ESCAPE_CHAR, (void *)&escapechar, FALSE);
		sprintf(idvalue, "%X", escapechar);
		*after_id_len=idtble->Idlen;
		*Value_Offset=idtble->Value_Offset;		
		}
		*/
		break;
	case Web_EscP_Id:
        /*
		{
		u16 EscapePeriod;
		tls_param_get(TLS_PARAM_ID_ESCAPE_PERIOD, (void *)&EscapePeriod, FALSE);
		sprintf(idvalue, "%d", EscapePeriod);
		*after_id_len=idtble->Idlen;
		*Value_Offset=idtble->Value_Offset;		
		}
		*/
		break;
	case Web_EscDataP_Id:
        /*
		{
		u16 PeriodT;
		tls_param_get(TLS_PARAM_ID_AUTO_TRIGGER_PERIOD, (void *)&PeriodT, FALSE);
		sprintf(idvalue, "%d", PeriodT);
		*after_id_len=idtble->Idlen;
		*Value_Offset=idtble->Value_Offset;		
		}
		*/
		break;
	case Web_EscDataL_Id:
        /*
		{
		u16 LengthT;
		tls_param_get(TLS_PARAM_ID_AUTO_TRIGGER_LENGTH, (void *)&LengthT, FALSE);
		sprintf(idvalue, "%d", LengthT);
		*after_id_len=idtble->Idlen;
		*Value_Offset=idtble->Value_Offset;		
		}
		*/
		break;	
#if 0		
	case Web_GPIO1_Id:
		  if(pSysVal->CfgPara.IOMode==0)
		  {
			strcpy(idvalue,"<option value=\"0\" selected=\"selected\">系统功能</option> <option value=\"1\">输入</option> <option value=\"2\">输出</option>\n");		
		  }
		  else if(pSysVal->CfgPara.IOMode==1)
		  {
			strcpy(idvalue,"<option value=\"0\" >系统功能</option> <option value=\"1\" selected=\"selected\">输入</option> <option value=\"2\">输出</option>\n");	
		  }
		  else
		  {
			strcpy(idvalue,"<option value=\"0\" >系统功能</option> <option value=\"1\" >输入</option> <option value=\"2\" selected=\"selected\">输出</option>\n");		
		  }
		*after_id_len=idtble->Idlen;
		*Value_Offset=idtble->Value_Offset;
		* Id_type=0x01;			
		break;
	case Web_CmdMode_Id:
		  if(pSysVal->CfgPara.CmdMode==0)
		  {
			strcpy(idvalue,"<option value=\"0\" selected=\"selected\">AT+指令集模式</option> <option value=\"1\">兼容协议模式</option>\n");		
		  }
		  else 
		  {
		       strcpy(idvalue,"<option value=\"0\" >AT+指令集模式</option> <option value=\"1\" selected=\"selected\">兼容协议模式</option>\n");  	
		  }
		*after_id_len=idtble->Idlen;
		*Value_Offset=idtble->Value_Offset;
		* Id_type=0x01;			
		break;		
#endif
	case Web_BssidEable_Id:
        /*
		tls_param_get(TLS_PARAM_ID_BSSID, (void *)&bssid, TRUE);
		tls_param_get(TLS_PARAM_ID_WPROTOCOL, (void* )&mode, FALSE);
		  if(mode==0)//sta
		  {
        	  if(bssid.bssid_enable==0)
        	  {
   				strcpy(idvalue,able_type[0]);	
        	  }
        	  else 
        	  {
   				strcpy(idvalue,able_type[1]);	
        	  }
		  }
		  else
		  {
			strcpy(idvalue,able_type[2]);	
		  }
		 	  
		*after_id_len=idtble->Idlen;
		*Value_Offset=idtble->Value_Offset;
		* Id_type=0x01;
		*/
		break;
	case Web_PassWord_Id:
        /*
		{
		u8 password[6];
              tls_param_get(TLS_PARAM_ID_PASSWORD, password, FALSE);
		sprintf(idvalue, "%c%c%c%c%c%c", \
			password[0],\
			password[1],\
			password[2],\
			password[3],\
			password[4],\
			password[5]);		
		*after_id_len=idtble->Idlen;
		*Value_Offset=idtble->Value_Offset;	
		}
		*/
		break;
	case Web_TCP_TimeOut_id:
        /*
		tls_param_get(TLS_PARAM_ID_AUTOMODE, (void *)&auto_mode, FALSE);
    		tls_param_get(TLS_PARAM_ID_DEFSOCKET, &remote_socket_cfg, FALSE);
		if(remote_socket_cfg.client_or_server && remote_socket_cfg.protocol == 0&&auto_mode) /// TCP Sever
		  {
			strcpy(idvalue, (char *)remote_socket_cfg.host);		
		  }
		  else 
		  {
				sprintf(idvalue, "0\" disabled=\"disabled\"");
				*after_id_len=idtble->Idlen;
				*Value_Offset=idtble->Value_Offset+1;	
				break;
		}

		*after_id_len=idtble->Idlen;
		*Value_Offset=idtble->Value_Offset;
		*/
		break;		
		
	case Web_SysInfo_Macaddr:
        /*
		{
		u8 *mac = wpa_supplicant_get_mac();
		sprintf(idvalue, "%02x-%02x-%02x-%02x-%02x-%02x", 	\
			mac[0],	\
			mac[1],	\
			mac[2],	\
			mac[3],	\
			mac[4],	\
			mac[5]);	
		*after_id_len=idtble->Idlen;
		*Value_Offset=idtble->Value_Offset;	
		}
		*/
		break;		
		
	case Web_SysInfo_HardVer:
        /*
		{
		struct tls_cmd_ver_t ver;
		tls_cmd_get_ver(&ver);
		sprintf(idvalue, "%c.%x.%02x.%02x.%02x%02x", \
			ver.hw_ver[0],
			ver.hw_ver[1],
			ver.hw_ver[2],
			ver.hw_ver[3],
			ver.hw_ver[4],
			ver.hw_ver[5]);
		*after_id_len=idtble->Idlen;
		*Value_Offset=idtble->Value_Offset;
		}
		*/
		break;	
		
	case Web_SysInfo_FirmVer:
        /*
		{
		struct tls_cmd_ver_t ver;
		tls_cmd_get_ver(&ver);
		sprintf(idvalue, "%c.%x.%02x.%02x", \
			ver.fw_ver[0], ver.fw_ver[1], ver.fw_ver[2], ver.fw_ver[3]);
		*after_id_len=idtble->Idlen;
		*Value_Offset=idtble->Value_Offset;	
		}
		*/
		break;	
		
	case Web_SysInfo_RelTime:
        /*
		sprintf(idvalue, "%s %s", \
			SysCreatedTime, SysCreatedDate);
		*after_id_len=idtble->Idlen;
		*Value_Offset=idtble->Value_Offset;
		*/
		break;		

	case Web_SiteInfo_Copyright:
        /*
		//if (!(pSysVal->CfgPara.DebugMode & DBGMODE_COPYRIGHT))
		{
			strcpy(idvalue, PRODUCT_MANUFACTOR);
			*after_id_len=idtble->Idlen;
			*Value_Offset=idtble->Value_Offset;	
		}
		*/
		break;		

	case Web_Try_times_id:
        /*
		{
		u8 auto_retry_cnt;
		tls_param_get(TLS_PARAM_ID_WPROTOCOL, (void* )&mode, FALSE);
		tls_param_get(TLS_PARAM_ID_AUTO_RETRY_CNT, (void* )&auto_retry_cnt, FALSE);
  		if(mode!=2)
         {
		    sprintf(idvalue, "%d", auto_retry_cnt);		
		    *after_id_len=idtble->Idlen;
		   *Value_Offset=idtble->Value_Offset;
        }
         else
		  {
		  	 strcpy(idvalue,"255\" size=\"3\" maxlength=\"3\" disabled=\"disabled\" />\n");  
		     *after_id_len=idtble->Idlen;
		     *Value_Offset=idtble->Value_Offset;
		     * Id_type=0x01;			
		 }
		}
		*/
		break;
	case Web_roam_id:
        /*
		{
		u8 roam;
		tls_param_get(TLS_PARAM_ID_WPROTOCOL, (void* )&mode, FALSE);
		tls_param_get(TLS_PARAM_ID_ROAMING, (void* )&roam, FALSE);
			if(mode==0)
			{
	    			if(roam)
	    			{
	    				strcpy(idvalue,able_type[1]);	
	    		
	    			}
	    			else    //////disable
	    			{
	    				strcpy(idvalue,able_type[0]);	
	    			}
			}
			else
			{
				strcpy(idvalue,able_type[2]);	
			}
			*after_id_len=idtble->Idlen;
			*Value_Offset=idtble->Value_Offset;
			* Id_type=0x01;	
		}
		*/
		break;
	case Web_powersave_id:
        /*
		{
		u8 autoPowerSave;
		tls_param_get(TLS_PARAM_ID_PSM, (void *)&autoPowerSave, FALSE);
		tls_param_get(TLS_PARAM_ID_WPROTOCOL, (void* )&mode, FALSE);
			if(mode==0)
			{
    			if(autoPowerSave)
    			{
    				strcpy(idvalue,able_type[1]);	
    		
    			}
    			else    //////disable
    			{
    				strcpy(idvalue,able_type[0]);	
    			}
			}
			else
			{
				strcpy(idvalue,able_type[2]);	
			}
			*after_id_len=idtble->Idlen;
			*Value_Offset=idtble->Value_Offset;
			* Id_type=0x01;	
		}
		*/
		break;
	case Web_SsidBroadcast_Id: /* ap mode */
        /*
		tls_param_get(TLS_PARAM_ID_WPROTOCOL, (void* )&mode, FALSE);
		if(mode==2)
		{
			u8 ssid_set;
			tls_param_get(TLS_PARAM_ID_BRDSSID, (void *)&ssid_set, FALSE);
			if(ssid_set)
			{
				strcpy(idvalue,able_type[1]);	
		
			}
			else	//////disable
			{
				strcpy(idvalue,able_type[0]);	
			}
		}
		else
		{
			strcpy(idvalue,able_type[2]);	
		}
		*after_id_len=idtble->Idlen;
		*Value_Offset=idtble->Value_Offset;
		* Id_type=0x01;
		*/
		break;

	case Web_Autocreateadhoc_Id:
        /*
		tls_param_get(TLS_PARAM_ID_WPROTOCOL, (void* )&mode, FALSE);
		if(mode==1) 
		{
			u8 auto_create_adhoc;
			tls_param_get(TLS_PARAM_ID_ADHOC_AUTOCREATE, (void *)&auto_create_adhoc, FALSE);
			if(auto_create_adhoc)
			{
				strcpy(idvalue,able_type[1]);	
			}
			else	//////disable
			{
				strcpy(idvalue,able_type[0]);	
			}
		}
		else
		{
			strcpy(idvalue,able_type[2]);	
		}
		
		*after_id_len=idtble->Idlen;
		*Value_Offset=idtble->Value_Offset;
		* Id_type=0x01;
		*/
		break;

		
    case Web_scan_id:
#if 0
		{
		   char *tempbuf=NULL;
		   char *tempbuf2=NULL;
		   int i, j, k, n;
		   u8 nettab[32];
			u8 channel;
			tls_param_get(TLS_PARAM_ID_CHANNEL, (void *)&channel, FALSE);
		   
			tempbuf = OSMMalloc(1536);
			if (tempbuf == NULL){break;}
			tempbuf2 = OSMMalloc(128);
			if (tempbuf2 == NULL){ OSMFree(tempbuf);break;}
			
			memset(tempbuf,0,1536);
			memset(tempbuf2,0,128);
			memset(nettab,0,sizeof(nettab));

			/* total number */
      		for(i=0;i<32;i++)
			{
         		if (channel <= 0){
         			break;
         		}
				nettab[i] = i;
      		};
			n = i;

      		for(i=0;i<n-1;i++)
			{
	      		for(j=0;j<n-i-1;j++)
				{
					if (pSysVal->Wifi.Aplist[nettab[j]].rSignalAgility < pSysVal->Wifi.Aplist[nettab[j+1]].rSignalAgility){
						k = nettab[j];
						nettab[j] = nettab[j+1];
						nettab[j+1] = k;
					}	
				}
      		}

			/* Limit the max display number */
			if (n > 16)
			 {
			   n=16;
			 }

			 sprintf(tempbuf, "<option value=\"%s\" selected=\"selected\">%s </option> ", \
			 	pSysVal->Wifi.Aplist[nettab[0]].SSID,pSysVal->Wifi.Aplist[nettab[0]].SSID);
			 for(i=1;i<n;i++)
			 {
			    sprintf(tempbuf2, "<option value=\"%s\">%s </option> ", \
					pSysVal->Wifi.Aplist[nettab[i]].SSID,pSysVal->Wifi.Aplist[nettab[i]].SSID);
				strcat(tempbuf,tempbuf2);
			 }
			strcpy(idvalue,tempbuf);
            OSMFree(tempbuf);
            OSMFree(tempbuf2);
		}
		*after_id_len=idtble->Idlen;
		*Value_Offset=idtble->Value_Offset;
		* Id_type=0x01;
#endif
		break;
   case Web_WebEncry_id:
        /*
#if 0
		strcpy(idvalue,Encry_Open[6]);
#endif
		strcpy(idvalue,Encry_Open[2]);
		*after_id_len=idtble->Idlen;
		*Value_Offset=idtble->Value_Offset;
		* Id_type=0x01;
		*/
	       break;		   
	default:
		break;
	}
	return(strlen(idvalue));
}



char *Res[] = {"/basic.html",  
			"/advance.html",	
			"/firmware.html",	
			"/index.html",	
#if WEB_SERVER_RUSSIAN		
			"/basic_en.html",	
			"/basic_ru.html",	
			"/firmware_en.html",
			"/firmware_ru.html",
#endif
};
static int HtmlConvertURLStr(char *drc, char *src, int len)
{
	int count;
	
	if((NULL == src) || (NULL == drc))
	{
		return -1;
	}
	count = len;
	while(count > 0)
	{
		if(0x2B == *src)	//space
		{
			*drc = 0x20;
			drc++;
			src++;
			count--;
		}
		else if(0x25 == *src)	// % 
		{
			src++;
			count--;
			strtohexarray((u8 *)drc, 1, src);
			drc++;
			src += 2;
			count -= 2;
		}
		else
		{
			*drc = *src;
			drc++;
			src++;
			count--;
		}
	}
	return 0;
}

#define CGI_CONFIG MK_CGI_ENTRY( \
	"/basic.html", \
	do_cgi_config \
	)

/*
static void set_default_socket(struct tls_param_socket *remote_socket_cfg)
{
#if TLS_CONFIG_HOSTIF
	struct tls_cmd_socket_t params;
	tls_cmd_get_default_socket_params(&params);
	params.client = remote_socket_cfg->client_or_server ? 0 : 1;
	params.proto = remote_socket_cfg->protocol;
	params.port = remote_socket_cfg->port_num;
	strcpy(params.host_name, (char *)remote_socket_cfg->host);
	if(remote_socket_cfg->client_or_server && remote_socket_cfg->protocol==0)//TCP server
	{
		string_to_uint((char *)remote_socket_cfg->host, &params.timeout);
	}
	tls_cmd_set_default_socket_params(&params, FALSE);
#endif
}
*/

extern u8 gucssidData[33];
extern u8 gucpwdData[65];

char * do_cgi_config(int iIndex, int iNumParams, char *pcParam[], char *pcValue[],u8 *  NeedRestart)
{
    /*
	int i;
	int Value;
///	int EnableAutoMode = 0;
	u32 Ip;
	int KeyType = 0;
	int KeyLen = 0;
	u8 encrypt;
	struct tls_param_ip ip_param;
	struct tls_param_key param_key;
	struct tls_param_original_key* orig_key;
	struct tls_param_sha1* sha1_key;
	u8 auto_mode;
	u8 mode;
	struct tls_param_socket remote_socket_cfg;
	*/
	
	int i;
	int KeyLen = 0;
    
    struct tls_param_ssid ssid;
	struct tls_param_key param_key;
	struct tls_param_original_key* orig_key;
	struct tls_param_sha1* sha1_key;

	if (iNumParams == 0)
	{
		* NeedRestart=0;
		return Res[iIndex];
	}

    /*
	for (i = 0; i < iNumParams; i++)
	{
		if (pcParam[i] == NULL || pcValue[i] == NULL || *pcValue[i] == '\0')
		{
			continue;
		}
		if (strcmp(pcParam[i], "restart") == 0)
		{
			//DBGPRINT("kevin debug do_cgi_config restart\n\r");
			* NeedRestart=1;	
			return Res[iIndex];
		}
	}
	*/
	
	//tls_param_get(TLS_PARAM_ID_IP, &ip_param, FALSE);
	//ip_param.dhcp_enable = 0;
	for (i = 0; i < iNumParams - 1; i++)		//not care about "Save"
	{
		//if (pcParam[i] == NULL || pcValue[i] == NULL || *pcValue[i] == '\0')
		if ((pcParam[i] == NULL) || (pcValue[i] == NULL) || 
			((*pcValue[i] == '\0') && (strcmp(pcParam[i], "Key") != 0)))
		{
			continue;
		}

        /*
		if (strcmp(pcParam[i], "Dhcp") == 0)
		{
			strtodec(&Value, pcValue[i]);	// 1
			ip_param.dhcp_enable = Value;
    			tls_param_set(TLS_PARAM_ID_IP, (void *)&ip_param, FALSE);
		}
		else if (strcmp(pcParam[i], "Ip") == 0)
		{

			strtoip(&Ip, pcValue[i]) ;
			ip_param.ip[0] = (Ip >> 24) & 0xFF;
			ip_param.ip[1] = (Ip >> 16) & 0xFF;
			ip_param.ip[2] = (Ip >> 8) & 0xFF;
			ip_param.ip[3] = Ip  & 0xFF;
    			tls_param_set(TLS_PARAM_ID_IP, (void *)&ip_param, FALSE);
		}
		else if (strcmp(pcParam[i], "Sub") == 0)
		{

			strtoip(&Ip, pcValue[i]) ;
			ip_param.netmask[0] = (Ip >> 24) & 0xFF;
			ip_param.netmask[1] = (Ip >> 16) & 0xFF;
			ip_param.netmask[2] = (Ip >> 8) & 0xFF;
			ip_param.netmask[3] = Ip  & 0xFF;
    			tls_param_set(TLS_PARAM_ID_IP, (void *)&ip_param, FALSE);
		}
		else if (strcmp(pcParam[i], "Gate") == 0)
		{

			strtoip(&Ip, pcValue[i]) ;
			ip_param.gateway[0] = (Ip >> 24) & 0xFF;
			ip_param.gateway[1] = (Ip >> 16) & 0xFF;
			ip_param.gateway[2] = (Ip >> 8) & 0xFF;
			ip_param.gateway[3] = Ip  & 0xFF;
    			tls_param_set(TLS_PARAM_ID_IP, (void *)&ip_param, FALSE);
		}
		else if (strcmp(pcParam[i], "Dns") == 0)
		{
			strtoip(&Ip, pcValue[i]) ;
			ip_param.dns1[0] = (Ip >> 24) & 0xFF;
			ip_param.dns1[1] = (Ip >> 16) & 0xFF;
			ip_param.dns1[2] = (Ip >> 8) & 0xFF;
			ip_param.dns1[3] = Ip  & 0xFF;
    			tls_param_set(TLS_PARAM_ID_IP, (void *)&ip_param, FALSE);
		}
		//else if (strcmp(pcParam[i], "Auto") == 0)
		//{
			//strtodec(&Value, pcValue[i]);	// 1
			//auto_mode = Value;
		//}
		else if(strcmp(pcParam[i], "AutoHiden")==0)
		{
			strtodec(&Value, pcValue[i]);
			//strtodec(&Value, pcValue[i]);
			auto_mode = Value;
			///DEBUG_PRINT("AutoHiden %d\n", Value);
			tls_param_set(TLS_PARAM_ID_AUTOMODE, (void *)&auto_mode, FALSE);
			tls_param_set(TLS_PARAM_ID_AUTO_RECONNECT, (void *)&auto_mode, FALSE);
		}
		else if (strcmp(pcParam[i], "Protocol") == 0)
		{
    			tls_param_get(TLS_PARAM_ID_DEFSOCKET, &remote_socket_cfg, FALSE);
			strtodec(&Value, pcValue[i]);
			//auto_mode=1;
			remote_socket_cfg.protocol = Value;
			//tls_param_set(TLS_PARAM_ID_AUTOMODE, (void *)&auto_mode, FALSE);
			//tls_param_set(TLS_PARAM_ID_AUTO_RECONNECT, (void *)&auto_mode, FALSE);
    			tls_param_set(TLS_PARAM_ID_DEFSOCKET, &remote_socket_cfg, FALSE);
			set_default_socket(&remote_socket_cfg);
		}
		else if (strcmp(pcParam[i], "Cs") == 0)
		{
    			tls_param_get(TLS_PARAM_ID_DEFSOCKET, &remote_socket_cfg, FALSE);
			strtodec(&Value, pcValue[i]);
			//auto_mode=1;
			
			remote_socket_cfg.client_or_server = Value;
			//tls_param_set(TLS_PARAM_ID_AUTOMODE, (void *)&auto_mode, FALSE);
			//tls_param_set(TLS_PARAM_ID_AUTO_RECONNECT, (void *)&auto_mode, FALSE);
    			tls_param_set(TLS_PARAM_ID_DEFSOCKET, &remote_socket_cfg, FALSE);
			set_default_socket(&remote_socket_cfg);
		}
		else if (strcmp(pcParam[i], "Domain") == 0)
		{
    			tls_param_get(TLS_PARAM_ID_DEFSOCKET, &remote_socket_cfg, FALSE);
			strcpy((char *)remote_socket_cfg.host, pcValue[i]);
			//auto_mode=1;

			//tls_param_set(TLS_PARAM_ID_AUTOMODE, (void *)&auto_mode, FALSE);
			//tls_param_set(TLS_PARAM_ID_AUTO_RECONNECT, (void *)&auto_mode, FALSE);
    			tls_param_set(TLS_PARAM_ID_DEFSOCKET, &remote_socket_cfg, FALSE);
			set_default_socket(&remote_socket_cfg);
		}
		else if (strcmp(pcParam[i], "TCP_TimeOut") == 0)
		{
    			tls_param_get(TLS_PARAM_ID_DEFSOCKET, &remote_socket_cfg, FALSE);
			int timeout=0;
			if (strtodec(&timeout,pcValue[i]) < 0)
			{
					continue;
			}
			if(timeout>10000000)
			{
			continue;
			}
			strcpy((char *)remote_socket_cfg.host, pcValue[i]);
			//auto_mode=1;
			//tls_param_set(TLS_PARAM_ID_AUTOMODE, (void *)&auto_mode, FALSE);
			//tls_param_set(TLS_PARAM_ID_AUTO_RECONNECT, (void *)&auto_mode, FALSE);
    			tls_param_set(TLS_PARAM_ID_DEFSOCKET, &remote_socket_cfg, FALSE);
			set_default_socket(&remote_socket_cfg);
		}			
		else if (strcmp(pcParam[i], "Port") == 0)
		{
    			tls_param_get(TLS_PARAM_ID_DEFSOCKET, &remote_socket_cfg, FALSE);
			strtodec(&Value, pcValue[i]);
			//auto_mode=1;

			remote_socket_cfg.port_num = Value;		
			//tls_param_set(TLS_PARAM_ID_AUTOMODE, (void *)&auto_mode, FALSE);	
			//tls_param_set(TLS_PARAM_ID_AUTO_RECONNECT, (void *)&auto_mode, FALSE);
    			tls_param_set(TLS_PARAM_ID_DEFSOCKET, &remote_socket_cfg, FALSE);
			set_default_socket(&remote_socket_cfg);
		}
		else if (strcmp(pcParam[i], "Ssid") == 0)
		{
			struct tls_param_ssid ssid;
			memset(&ssid, 0, sizeof(struct tls_param_ssid));
			HtmlConvertURLStr((char *)ssid.ssid, pcValue[i], strlen(pcValue[i]));
			ssid.ssid_len = strlen((char *)ssid.ssid);
			tls_param_set(TLS_PARAM_ID_SSID, (void *)&ssid, FALSE);
		} 
		else if (strcmp(pcParam[i], "Encry") == 0)
		{
			tls_param_get(TLS_PARAM_ID_ENCRY, (void *)&encrypt, FALSE);
			strtodec(&Value, pcValue[i]);
			if(Value==0)  ///open
			{
				encrypt=ENCRYPT_TYPE_OPEN_SYS;
				//pSysVal->CfgPara.AuthMode=0;
			}
			else if(Value==1)///
			{
				encrypt=ENCRYPT_TYPE_WEP_64_BITS;
				//pSysVal->CfgPara.AuthMode=0;		
			}
			else if(Value==2)///
			{
				encrypt=ENCRYPT_TYPE_WEP_128_BITS;
				//pSysVal->CfgPara.AuthMode=0;		
			}	
#if 1
			else if(Value==3)///
			{
				encrypt=3;
				//pSysVal->CfgPara.AuthMode=AUTHMODE_WPA;		
			}	
			else if(Value==4)///
			{
				encrypt=4;
				//pSysVal->CfgPara.AuthMode=AUTHMODE_WPA;		
			}				
			else if(Value==5)///
			{
				encrypt=5;
				//pSysVal->CfgPara.AuthMode=AUTHMODE_WPA2;		
			}				
			else if(Value==6)///
			{
				encrypt=6;
				//pSysVal->CfgPara.AuthMode=AUTHMODE_WPA2;		
			}	
#endif
			else
			{

			}
			tls_param_set(TLS_PARAM_ID_ENCRY, (void *)&encrypt, FALSE);
		}
		else if (strcmp(pcParam[i], "KeyType") == 0)
		{
			tls_param_get(TLS_PARAM_ID_KEY, (void *)&param_key, FALSE);
			strtodec(&KeyType, pcValue[i]);
			param_key.key_format =KeyType;
			tls_param_set(TLS_PARAM_ID_KEY, (void *)&param_key, FALSE);
		}
		else if (strcmp(pcParam[i], "KeyIndex") == 0)
		{
			tls_param_get(TLS_PARAM_ID_KEY, (void *)&param_key, FALSE);
			strtodec(&Value, pcValue[i]);
			param_key.key_index = Value;
			tls_param_set(TLS_PARAM_ID_KEY, (void *)&param_key, FALSE);
		}
		else if (strcmp(pcParam[i], "Key") == 0)
		{
			tls_param_get(TLS_PARAM_ID_KEY, (void *)&param_key, FALSE);
			memset(param_key.psk, 0, sizeof(param_key.psk));
			HtmlConvertURLStr((char *)param_key.psk, pcValue[i], strlen(pcValue[i]));
			KeyLen = strlen((char *)param_key.psk);
			param_key.key_length = KeyLen;
			//DBGPRINT("### kevin debug web key = %s\n\r",param_key.psk);
			tls_param_set(TLS_PARAM_ID_KEY, (void *)&param_key, FALSE);

			orig_key = (struct tls_param_original_key*)&param_key;
            HtmlConvertURLStr((char *)orig_key->psk, pcValue[i], strlen(pcValue[i]));
            orig_key->key_length = strlen((char *)param_key.psk);
            tls_param_set(TLS_PARAM_ID_ORIGIN_KEY, (void *)orig_key, FALSE);

            sha1_key = (struct tls_param_sha1*)&param_key;
            memset((u8* )sha1_key, 0, sizeof(struct tls_param_sha1));
            tls_param_set(TLS_PARAM_ID_SHA1, (void *)sha1_key, FALSE);
		}	
		else if (strcmp(pcParam[i], "Mode") == 0)
		{
			tls_param_get(TLS_PARAM_ID_ENCRY, (void *)&encrypt, FALSE);
			strtodec(&Value, pcValue[i]);
			mode = Value;
			if(mode==0)
			{
				u8 auto_create_adhoc=0;
				tls_param_set(TLS_PARAM_ID_ADHOC_AUTOCREATE, (void *)&auto_create_adhoc, FALSE);
			}
			if(IEEE80211_MODE_INFRA == mode)
			{
				//encrypt = ENCRYPT_TYPE_CCMP;
				//pSysVal->CfgPara.AuthMode = AUTHMODE_WPA2;
				ip_param.dhcp_enable = 1;
    				tls_param_set(TLS_PARAM_ID_IP, (void *)&ip_param, FALSE);
			}
#if WEB_SERVER_RUSSIAN
			else if(IEEE80211_MODE_AP == mode)
			{
				encrypt = ENCRYPT_TYPE_OPEN_SYS;
				//pSysVal->CfgPara.AuthMode = AUTHMODE_AUTO;
				ip_param.dhcp_enable = 0;
    				tls_param_set(TLS_PARAM_ID_IP, (void *)&ip_param, FALSE);
			}
#endif
			tls_param_set(TLS_PARAM_ID_WPROTOCOL, (void* )&mode, FALSE);
			tls_param_set(TLS_PARAM_ID_ENCRY, (void *)&encrypt, FALSE);
		}
		else if (strcmp(pcParam[i], "DnsName") == 0)
		{
			u8 dnsname[32];
			memset(dnsname, 0, sizeof(dnsname));
			HtmlConvertURLStr((char *)dnsname, pcValue[i], strlen(pcValue[i]));
			tls_param_set(TLS_PARAM_ID_DNSNAME, dnsname, FALSE);
		}
		*/

        if (strcmp(pcParam[i], "Ssid") == 0)
		{
			memset(&ssid, 0, sizeof(struct tls_param_ssid));
			HtmlConvertURLStr((char *)ssid.ssid, pcValue[i], strlen(pcValue[i]));
            ssid.ssid_len = strlen(pcValue[i]);
            memset(gucssidData, 0, sizeof(gucssidData));
            memcpy(gucssidData, ssid.ssid, ssid.ssid_len);
			tls_param_set(TLS_PARAM_ID_SSID, (void *)&ssid, FALSE);
		} 
        else if (strcmp(pcParam[i], "Key") == 0)
		{
			u8 *pskinfo = NULL;
			pskinfo = tls_mem_alloc(128);
			if (pskinfo)
			{
				tls_param_get(TLS_PARAM_ID_KEY, (void *)&param_key, FALSE);
				memset(param_key.psk, 0, sizeof(param_key.psk));
				memset(pskinfo, 0, 128);
				HtmlConvertURLStr((char *)pskinfo, pcValue[i], strlen(pcValue[i]));
			    DEBUG_PRINT("### kevin debug web key = %s\n\r",pskinfo);

				KeyLen = strlen((char *)pskinfo);

				param_key.key_length = KeyLen;
				memcpy(param_key.psk, pskinfo, KeyLen);
	            strcpy((char *)gucpwdData, (char *)pskinfo);
				tls_param_set(TLS_PARAM_ID_KEY, (void *)&param_key, FALSE);

				orig_key = (struct tls_param_original_key*)&param_key;
				memset(orig_key, 0, sizeof(struct tls_param_original_key));
				orig_key->key_length = KeyLen;
				memcpy(orig_key->psk, pskinfo, KeyLen);
	           	tls_param_set(TLS_PARAM_ID_ORIGIN_KEY, (void *)orig_key, FALSE);

	            sha1_key = (struct tls_param_sha1*)&param_key;
	            memset((u8* )sha1_key, 0, sizeof(struct tls_param_sha1));
	            tls_param_set(TLS_PARAM_ID_SHA1, (void *)sha1_key, FALSE);		
				tls_mem_free(pskinfo);
			}
		}
	}
	if (gwebcfgmode == 0)
	{
    //set sta mode and reset the system.
    	tls_param_to_flash(TLS_PARAM_ID_ALL);
#if TLS_CONFIG_WEB_SERVER_MODE
		gwebcfgmode = 1;    
#endif
	}
    /*
    httpd_deinit();
    tls_wifi_oneshot_connect(ssid.ssid, param_key.psk);
    */
	return Res[iIndex];
}

#define CGI_ADVANCE MK_CGI_ENTRY( \
	"/advance.html", \
	do_cgi_advance \
	)
	
char * do_cgi_advance(int iIndex, int iNumParams, char *pcParam[], char *pcValue[], u8 * NeedRestart)
{
    /*
	int i;
	int Value;
	u32 HexValue;
	int EscapePeriod = 0;
	int Period = 0;
	int Length = 0;
	int Port = 0;
	int Chll = 0;
	u8 PassWord[6];
	struct tls_param_bgr wbgr;
	struct tls_param_uart uart_cfg;
	if (iNumParams == 0)
	{
	      * NeedRestart=0;
		return Res[iIndex];
	}
	
    
	for (i = 0; i < iNumParams - 1; i++)		//not care about "Save"
	{
		if (pcParam[i] == NULL || pcValue[i] == NULL || *pcValue[i] == '\0')
		{
			continue;
		}
		
		if (strcmp(pcParam[i], "Baud") == 0)
		{
			tls_param_get(TLS_PARAM_ID_UART, (void *)&uart_cfg, FALSE);
			strtodec(&Value, pcValue[i]);
			uart_cfg.baudrate = Web_Baud[Value];
			tls_param_set(TLS_PARAM_ID_UART, (void *)&uart_cfg, FALSE);
		} 
		else if (strcmp(pcParam[i], "Check") == 0)
		{
			tls_param_get(TLS_PARAM_ID_UART, (void *)&uart_cfg, FALSE);
			strtodec(&Value, pcValue[i]);		// 1
			uart_cfg.parity = Value;
			tls_param_set(TLS_PARAM_ID_UART, (void *)&uart_cfg, FALSE);
		}
		else if (strcmp(pcParam[i], "DataSize") == 0)
		{
			tls_param_get(TLS_PARAM_ID_UART, (void *)&uart_cfg, FALSE);
			strtodec(&Value, pcValue[i]);
			//kevin modify 20131227 only 8 bit 
			uart_cfg.charsize = 0;
			tls_param_set(TLS_PARAM_ID_UART, (void *)&uart_cfg, FALSE);
		}
		else if (strcmp(pcParam[i], "StopSize") == 0)
		{
			tls_param_get(TLS_PARAM_ID_UART, (void *)&uart_cfg, FALSE);
			strtodec(&Value, pcValue[i]);
			uart_cfg.stop_bits = Value;
			tls_param_set(TLS_PARAM_ID_UART, (void *)&uart_cfg, FALSE);
		}		
		else if (strcmp(pcParam[i], "WebPort") == 0)
		{
			struct tls_webs_cfg webcfg;
			tls_param_get(TLS_PARAM_ID_WEBS_CONFIG, (void *)&webcfg, FALSE);
			strtodec(&Port, pcValue[i]);
			webcfg.PortNum = Port;
			tls_param_set(TLS_PARAM_ID_WEBS_CONFIG, (void *)&webcfg, FALSE);
			
		}		
		else if (strcmp(pcParam[i], "EscChar") == 0)
		{
			u8 escapechar;
			strtohex(&HexValue, pcValue[i]);
			escapechar = HexValue;
			tls_param_set(TLS_PARAM_ID_ESCAPE_CHAR, (void *)&escapechar, FALSE);
		}
		else if (strcmp(pcParam[i], "EscP") == 0)
		{
			u16 PeriodT;
			tls_param_get(TLS_PARAM_ID_AUTO_TRIGGER_PERIOD, (void *)&PeriodT, FALSE);
			strtodec(&EscapePeriod, pcValue[i]);
			
			if(100 <= EscapePeriod <= 10000){
				EscapePeriod = (EscapePeriod/100)*100;
				if (EscapePeriod > PeriodT){
					tls_param_set(TLS_PARAM_ID_ESCAPE_PERIOD, (void *)&EscapePeriod, FALSE);
				}
			}
		}
		else if (strcmp(pcParam[i], "EscDataP") == 0)
		{
			tls_param_set(TLS_PARAM_ID_ESCAPE_PERIOD, (void *)&EscapePeriod, FALSE);
			strtodec(&Period, pcValue[i]);	
			
			if(0 <= Period <= 10000){
				if (Period < EscapePeriod){
					tls_param_set(TLS_PARAM_ID_AUTO_TRIGGER_PERIOD, (void *)&Period, FALSE);
				}
			}
		}		
		else if (strcmp(pcParam[i], "EscDataL") == 0)
		{
			strtodec(&Length, pcValue[i]);	
			if(32 <= Length <= 1024){
				tls_param_set(TLS_PARAM_ID_AUTO_TRIGGER_LENGTH, (void *)&Length, FALSE);
			}
		}	
		else if (strcmp(pcParam[i], "PassWord") == 0)
		{
			memset(PassWord,0,6);
			strncpy((char *)PassWord, pcValue[i],6);
			tls_param_set(TLS_PARAM_ID_PASSWORD,  PassWord, FALSE);
		}
		//else if(strcmp(pcParam[i], "AdhocHiden")==0)
		else if(strcmp(pcParam[i], "autocreateadhoc")==0)
		{
			u8 auto_create_adhoc;
			strtodec(&Value, pcValue[i]);
			auto_create_adhoc=Value;
			tls_param_set(TLS_PARAM_ID_ADHOC_AUTOCREATE, (void *)&auto_create_adhoc, FALSE);
		}
		else if (strcmp(pcParam[i], "Bg") == 0)
		{
			tls_param_get(TLS_PARAM_ID_WBGR, (void *)&wbgr, FALSE);
			strtodec(&Value, pcValue[i]);
			wbgr.bg = Value;
			tls_param_set(TLS_PARAM_ID_WBGR, (void *)&wbgr, FALSE);
		}
		else if (strcmp(pcParam[i], "Rate") == 0)
		{
			tls_param_get(TLS_PARAM_ID_WBGR, (void *)&wbgr, FALSE);
			strtodec(&Value, pcValue[i]);
			wbgr.max_rate = Value;
			tls_param_set(TLS_PARAM_ID_WBGR, (void *)&wbgr, FALSE);
		}
		
		else if (strcmp(pcParam[i], "roam") == 0)
		{
			u8 roam;
			strtodec(&Value, pcValue[i]);
			roam = Value;
			tls_param_set(TLS_PARAM_ID_ROAMING, (void* )&roam, FALSE);
		}//roam	

		else if (strcmp(pcParam[i], "powersave") == 0)
		{
			u8 autoPowerSave;
			strtodec(&Value, pcValue[i]);
			autoPowerSave = Value;
			tls_param_set(TLS_PARAM_ID_PSM, (void *)&autoPowerSave, FALSE);
		}

		else if (strcmp(pcParam[i], "Try_times") == 0)
		{
			strtodec(&Value, pcValue[i]);
			if(Value<=255)
			{
				u8 auto_retry_cnt = Value;
				tls_param_set(TLS_PARAM_ID_AUTO_RETRY_CNT, (void* )&auto_retry_cnt, FALSE);
			}
		}		
		else if (strcmp(pcParam[i], "Bssid") == 0)
		{
			struct tls_param_bssid bssid;
			tls_param_get(TLS_PARAM_ID_BSSID, (void *)&bssid, FALSE);
			strtohexarray(bssid.bssid, 6, pcValue[i]);			
			tls_param_set(TLS_PARAM_ID_BSSID, (void *)&bssid, FALSE);
		}
		else if(strcmp(pcParam[i], "BssidEable") == 0)
		{
			struct tls_param_bssid bssid;
			tls_param_get(TLS_PARAM_ID_BSSID, (void *)&bssid, FALSE);
			strtodec(&Value, pcValue[i]);
			bssid.bssid_enable = Value;	// 1
			tls_param_set(TLS_PARAM_ID_BSSID, (void *)&bssid, FALSE);
		}
		else if (strcmp(pcParam[i], "Channel") == 0)
		{
			u8 channel;
			u8 channel_en;
			strtodec(&Value, pcValue[i]);
			if (Value == 0)	//Auto
			{				
				channel_en = Value;	
			}
			else
			{
				channel_en = 1;
				channel = Value;
				tls_param_set(TLS_PARAM_ID_CHANNEL, (void *)&channel, FALSE);
			}
			tls_param_set(TLS_PARAM_ID_CHANNEL_EN, (void *)&channel_en, FALSE);
		}	
		//Place to the final, by wangyf 
		else if ((strncmp(pcParam[i], "Ch", 2) == 0)&&(*(pcParam[i]+2) >= '0')&&(*(pcParam[i]+2) <= '9'))
		{
			strtodec(&Value, pcParam[i]+2);
			if ((Value >= 1)&&(Value <= 14)){
				Chll |= (1<<(Value-1));
			}
		} 
	}
	
	if(Chll!=0){
		tls_param_set(TLS_PARAM_ID_CHANNEL_LIST, (void *)&Chll, FALSE);
	}
	
	tls_param_to_flash(TLS_PARAM_ID_ALL);
	*/
	
	return Res[iIndex];
}

#define CGI_CONTROL MK_CGI_ENTRY( \
	"/firmware.html", \
	do_cgi_firmware \
	)

char * do_cgi_firmware(int iIndex, int iNumParams, char *pcParam[], char *pcValue[], u8 * NeedRestart)
{
	//int i;

	//DBGPRINT("kevin debug do_cgi_firmware %x\n\r",iNumParams);
	if (iNumParams == 0){
	      * NeedRestart=0;
		return Res[iIndex];
	}	

    /*
	for (i = 0; i < iNumParams; i++)
	{
		//DBGPRINT("kevin debug do_cgi_firmware %s\n\r",pcParam[0]);
		if (pcParam[i] == NULL || pcValue[i] == NULL || *pcValue[i] == '\0')
		{
			continue;
		}
					
		if (strcmp(pcParam[i], "restart") == 0)
		{
			* NeedRestart=1;	
		}
	}
	*/
	
	return Res[iIndex];
}

char * do_cgi_webindex(int iIndex, int iNumParams, char *pcParam[], char *pcValue[], u8 * NeedRestart)
{
		int i;
		int Value;
		//int KeyLen = 0;	
		u8 encrypt;
		//struct tls_param_key param_key;
		if (iNumParams == 0)
		{
			* NeedRestart=0;
			return Res[iIndex];
		}
		for (i = 0; i < iNumParams - 1; i++)		//not care about "Save"
		{
			if (pcParam[i] == NULL || pcValue[i] == NULL || *pcValue[i] == '\0')
			{
				continue;
			}
    		 if (strcmp(pcParam[i], "SsidSL") == 0)
    		{
			struct tls_param_ssid ssid;
			memset(&ssid, 0, sizeof(struct tls_param_ssid));
			HtmlConvertURLStr((char *)ssid.ssid, pcValue[i], strlen(pcValue[i]));
			ssid.ssid_len = strlen(pcValue[i]);
			tls_param_set(TLS_PARAM_ID_SSID, (void *)&ssid, FALSE);
    		} 
     		else if (strcmp(pcParam[i], "Encryweb") == 0)
     		{
			tls_param_get(TLS_PARAM_ID_ENCRY, (void *)&encrypt, FALSE);
     			strtodec(&Value, pcValue[i]);
     			if(Value==0)  ///open
     			{
     				encrypt=ENCRYPT_TYPE_OPEN_SYS;
     				//pSysVal->CfgPara.AuthMode=0;
     			}
     			else if(Value==1)///
     			{
     				encrypt=ENCRYPT_TYPE_WEP_64_BITS;
     				//pSysVal->CfgPara.AuthMode=0;		
     			}
     			else if(Value==2)///
     			{
     				encrypt=ENCRYPT_TYPE_WEP_128_BITS;
     				//pSysVal->CfgPara.AuthMode=0;		
     			}	
#if 0
     			else if(Value==3)///
     			{
     				encrypt=ENCRYPT_TYPE_TKIP;
     				pSysVal->CfgPara.AuthMode=AUTHMODE_WPA;		
     			}	
     			else if(Value==4)///
     			{
     				encrypt=ENCRYPT_TYPE_CCMP;
     				pSysVal->CfgPara.AuthMode=AUTHMODE_WPA;		
     			}				
     			else if(Value==5)///
     			{
     				encrypt=ENCRYPT_TYPE_TKIP;
     				pSysVal->CfgPara.AuthMode=AUTHMODE_WPA2;		
     			}				
     			else if(Value==6)///
     			{
     				encrypt=ENCRYPT_TYPE_CCMP;
     				pSysVal->CfgPara.AuthMode=AUTHMODE_WPA2;		
     			}	
#endif
     			else
     			{
     
     			}
			tls_param_set(TLS_PARAM_ID_ENCRY, (void *)&encrypt, FALSE);
     		}
			else if(strcmp(pcParam[i], "ApEnableweb") == 0)
		    {
			    //strtodec(&Value, pcValue[i]);
			    u8 mode= 0;	// 1
			    tls_param_get(TLS_PARAM_ID_WPROTOCOL, (void* )&mode, FALSE);
		    }	
		}			
		tls_param_to_flash(TLS_PARAM_ID_ALL);
		* NeedRestart=1;	
		return Res[iIndex];

}

#define CGI_WEBINDEX MK_CGI_ENTRY( \
	"/index.html", \
	do_cgi_webindex \
	)
#if WEB_SERVER_RUSSIAN
#define CGI_CONFIG_EN MK_CGI_ENTRY( \
	"/basic_en.html", \
	do_cgi_config \
	)
#define CGI_CONFIG_RU MK_CGI_ENTRY( \
	"/basic_ru.html", \
	do_cgi_config \
	)
#define CGI_FIRMWARE_EN MK_CGI_ENTRY( \
	"/firmware_en.html", \
	do_cgi_firmware \
	)
#define CGI_FIRMWARE_RU MK_CGI_ENTRY( \
	"/firmware_ru.html", \
	do_cgi_firmware \
	)	
#endif		

tCGI Cgi[8]= 
{
	CGI_CONFIG,
	CGI_ADVANCE,
	CGI_CONTROL,
	CGI_WEBINDEX,	
#if WEB_SERVER_RUSSIAN	
	CGI_CONFIG_EN,
	CGI_CONFIG_RU,
	CGI_FIRMWARE_EN,
	CGI_FIRMWARE_RU,
#endif	
};

