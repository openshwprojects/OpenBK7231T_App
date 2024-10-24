/**
 * @file    wm_ieee80211.h
 *
 * @brief   IEEE802.11 protocol type define
 *
 * @author  winnermicro
 *
 * Copyright (c) 2014 Winner Microelectronics Co., Ltd.
 */
#ifndef WM_IEEE80211_H
#define WM_IEEE80211_H

/*
 * DS bit usage
 *
 * TA = transmitter address
 * RA = receiver address
 * DA = destination address
 * SA = source address
 *
 * ToDS    FromDS  A1(RA)  A2(TA)  A3      A4      Use
 * -----------------------------------------------------------------
 *  0       0       DA      SA      BSSID   -       IBSS/DLS
 *  0       1       DA      BSSID   SA      -       AP -> STA
 *  1       0       BSSID   SA      DA      -       AP <- STA
 *  1       1       RA      TA      DA      SA      unspecified (WDS)
 */

#define FCS_LEN 4

#define IEEE80211_FCTL_VERS         0x0003
#define IEEE80211_FCTL_FTYPE        0x000c
#define IEEE80211_FCTL_STYPE        0x00f0
#define IEEE80211_FCTL_TODS         0x0100
#define IEEE80211_FCTL_FROMDS		0x0200
#define IEEE80211_FCTL_MOREFRAGS	0x0400
#define IEEE80211_FCTL_RETRY		0x0800
#define IEEE80211_FCTL_PM		0x1000
#define IEEE80211_FCTL_MOREDATA		0x2000
#define IEEE80211_FCTL_PROTECTED	0x4000
#define IEEE80211_FCTL_ORDER		0x8000

#define IEEE80211_SCTL_FRAG		0x000F
#define IEEE80211_SCTL_SEQ		0xFFF0

#define IEEE80211_FTYPE_MGMT		0x0000
#define IEEE80211_FTYPE_CTL		0x0004
#define IEEE80211_FTYPE_DATA		0x0008

/* management */
#define IEEE80211_STYPE_ASSOC_REQ	0x0000
#define IEEE80211_STYPE_ASSOC_RESP	0x0010
#define IEEE80211_STYPE_REASSOC_REQ	0x0020
#define IEEE80211_STYPE_REASSOC_RESP	0x0030
#define IEEE80211_STYPE_PROBE_REQ	0x0040
#define IEEE80211_STYPE_PROBE_RESP	0x0050
#define IEEE80211_STYPE_BEACON		0x0080
#define IEEE80211_STYPE_ATIM		0x0090
#define IEEE80211_STYPE_DISASSOC	0x00A0
#define IEEE80211_STYPE_AUTH		0x00B0
#define IEEE80211_STYPE_DEAUTH		0x00C0
#define IEEE80211_STYPE_ACTION		0x00D0

/* control */
#define IEEE80211_STYPE_BACK_REQ	0x0080
#define IEEE80211_STYPE_BACK		0x0090
#define IEEE80211_STYPE_PSPOLL		0x00A0
#define IEEE80211_STYPE_RTS		0x00B0
#define IEEE80211_STYPE_CTS		0x00C0
#define IEEE80211_STYPE_ACK		0x00D0
#define IEEE80211_STYPE_CFEND		0x00E0
#define IEEE80211_STYPE_CFENDACK	0x00F0

/* data */
#define IEEE80211_STYPE_DATA			0x0000
#define IEEE80211_STYPE_DATA_CFACK		0x0010
#define IEEE80211_STYPE_DATA_CFPOLL		0x0020
#define IEEE80211_STYPE_DATA_CFACKPOLL		0x0030
#define IEEE80211_STYPE_NULLFUNC		0x0040
#define IEEE80211_STYPE_CFACK			0x0050
#define IEEE80211_STYPE_CFPOLL			0x0060
#define IEEE80211_STYPE_CFACKPOLL		0x0070
#define IEEE80211_STYPE_QOS_DATA		0x0080
#define IEEE80211_STYPE_QOS_DATA_CFACK		0x0090
#define IEEE80211_STYPE_QOS_DATA_CFPOLL		0x00A0
#define IEEE80211_STYPE_QOS_DATA_CFACKPOLL	0x00B0
#define IEEE80211_STYPE_QOS_NULLFUNC		0x00C0
#define IEEE80211_STYPE_QOS_CFACK		0x00D0
#define IEEE80211_STYPE_QOS_CFPOLL		0x00E0
#define IEEE80211_STYPE_QOS_CFACKPOLL		0x00F0


#define IEEE80211_STA_DEFAULT_LISTEN_INTERVAL 3
#define IEEE80211_STA_MIN_LISTEN_INTERVAL     1

/* miscellaneous IEEE 802.11 constants */
#define IEEE80211_MAX_FRAG_THRESHOLD	    2352
#define IEEE80211_MAX_RTS_THRESHOLD	        2353
#define IEEE80211_MAX_AID		            2007
#define IEEE80211_MAX_TIM_LEN		        251
/* Maximum size for the MA-UNITDATA primitive, 802.11 standard section
   6.2.1.1.2.

   802.11e clarifies the figure in section 7.1.2. The frame body is
   up to 2304 octets long (maximum MSDU size) plus any crypt overhead. */
#define IEEE80211_MAX_DATA_LEN		2304
/* 30 byte 4 addr hdr, 2 byte QoS, 2304 byte MSDU, 12 byte crypt, 4 byte FCS */
#define IEEE80211_MAX_FRAME_LEN		2352

#define IEEE80211_MAX_SSID_LEN		32

#define IEEE80211_MAX_MESH_ID_LEN	32

#define IEEE80211_QOS_CTL_LEN		2
#define IEEE80211_QOS_CTL_TID_MASK	0x000F
#define IEEE80211_QOS_CTL_TAG1D_MASK	0x0007

#define IEEE80211_HT_CTL_LEN		4

/* U-APSD queue for WMM IEs sent by AP */
#define IEEE80211_WMM_IE_AP_QOSINFO_UAPSD	(1<<7)

/* U-APSD queues for WMM IEs sent by STA */
#define IEEE80211_WMM_IE_STA_QOSINFO_AC_VO	(1<<0)
#define IEEE80211_WMM_IE_STA_QOSINFO_AC_VI	(1<<1)
#define IEEE80211_WMM_IE_STA_QOSINFO_AC_BK	(1<<2)
#define IEEE80211_WMM_IE_STA_QOSINFO_AC_BE	(1<<3)
#define IEEE80211_WMM_IE_STA_QOSINFO_AC_MASK	0x0f

/* U-APSD max SP length for WMM IEs sent by STA */
#define IEEE80211_WMM_IE_STA_QOSINFO_SP_ALL	0x00
#define IEEE80211_WMM_IE_STA_QOSINFO_SP_2	0x01
#define IEEE80211_WMM_IE_STA_QOSINFO_SP_4	0x02
#define IEEE80211_WMM_IE_STA_QOSINFO_SP_6	0x03
#define IEEE80211_WMM_IE_STA_QOSINFO_SP_MASK	0x03
#define IEEE80211_WMM_IE_STA_QOSINFO_SP_SHIFT	5

/* Parsed Information Elements */
struct ieee802_11_elems {
    u8 *ie_start;
    size_t total_len;

/* pointers to IEs */
    u8 *ssid;
    u8 *supp_rates;
    u8 *fh_params;
    u8 *ds_params;
    u8 *cf_params;
    u8 *tim;
    u8 *ibss_params;
    u8 *country_elem;
    u8 *challenge;
    u8 *erp_info;
    u8 *ext_supp_rates;
    u8 *wpa_ie;
    u8 *rsn_ie;
    u8 *wmm;                    /* WMM Information or Parameter Element */
    u8 *wmm_param;
    struct ieee80211_ht_cap *ht_cap_elem;
    struct ieee80211_ht_info *ht_info_elem;
    u8 *wmm_tspec;
    u8 *wps_ie;
    u8 *power_cap;
    u8 *supp_channels;
    u8 *mdie;
    u8 *ftie;
    u8 *timeout_int;
    u8 *ch_switch_elem;
    u8 *ht_capabilities;
    u8 *ht_operation;
    u8 *vendor_ht_cap;
    u8 *p2p;
    u8 *link_id;
    u8 *pwr_constr_elem;
    u8 *interworking;

    u8 ssid_len;
    u8 supp_rates_len;
    u8 fh_params_len;
    u8 ds_params_len;
    u8 cf_params_len;
    u8 tim_len;
    u8 ibss_params_len;
    u8 challenge_len;
    u8 erp_info_len;
    u8 ext_supp_rates_len;
    u8 ch_switch_elem_len;
    u8 wpa_ie_len;
    u8 rsn_ie_len;
    u8 wmm_len;                 /* 7 = WMM Information; 24 = WMM Parameter */
    u8 wmm_param_len;
    u8 wmm_tspec_len;
    u8 wps_ie_len;
    u8 power_cap_len;
    u8 supp_channels_len;
    u8 mdie_len;
    u8 ftie_len;
    u8 timeout_int_len;
    u8 ht_capabilities_len;
    u8 pwr_constr_elem_len;
    u8 country_elem_len;
    u8 ht_operation_len;
    u8 vendor_ht_cap_len;
    u8 p2p_len;
    u8 interworking_len;

};

 __packed struct ieee80211_hdr {
    u16 frame_control;
    u16 duration_id;
    u8 addr1[6];
    u8 addr2[6];
    u8 addr3[6];
    u16 seq_ctrl;
    u8 addr4[6];
};

 __packed struct ieee80211_hdr_3addr {
    u16 frame_control;
    u16 duration_id;
    u8 addr1[6];
    u8 addr2[6];
    u8 addr3[6];
    u16 seq_ctrl;
};

 __packed struct ieee80211_qos_hdr {
    u16 frame_control;
    u16 duration_id;
    u8 addr1[6];
    u8 addr2[6];
    u8 addr3[6];
    u16 seq_ctrl;
    u16 qos_ctrl;
};

typedef enum _phy_type{
    phy_80211_b,
    phy_80211_bg,
    phy_80211_bgn,
    phy_80211_n,
    phy_80211_max
} phy_type;


#define IEEE80211_HDRLEN (sizeof(struct ieee80211_hdr_3addr))

/**
 * @brief          check if IEEE80211_FCTL_TODS is set
 *
 * @param[in]      fc    frame control bytes in little-endian byteorder
 *
 * @retval         0     no  tods
 * @retval         other has tods
 *
 * @note           None
 */
static __inline int ieee80211_has_tods(u16 fc)
{
    return (fc & IEEE80211_FCTL_TODS) != 0;
}

/**
 * @brief          check if IEEE80211_FCTL_FROMDS is set
 *
 * @param[in]      fc    frame control bytes in little-endian byteorder
 *
 * @retval         0     no  fromds
 * @retval         other has fromds
 *
 * @note           None
 */
static __inline int ieee80211_has_fromds(u16 fc)
{
    return (fc & IEEE80211_FCTL_FROMDS) != 0;
}

/**
 * @brief          check if IEEE80211_FCTL_TODS and IEEE80211_FCTL_FROMDS are set
 *
 * @param[in]      fc    frame control bytes in little-endian byteorder
 *
 * @retval         0     no  a4
 * @retval         other has a4
 *
 * @note           None
 */
static __inline int ieee80211_has_a4(u16 fc)
{
    u16 tmp = IEEE80211_FCTL_TODS | IEEE80211_FCTL_FROMDS;
    return (fc & tmp) == tmp;
}

/**
 * @brief          check if IEEE80211_FCTL_MOREFRAGS is set
 *
 * @param[in]      fc    frame control bytes in little-endian byteorder
 *
 * @retval         0     no  morefrags
 * @retval         other has morefrags
 *
 * @note           None
 */
static __inline int ieee80211_has_morefrags(u16 fc)
{
    return (fc & IEEE80211_FCTL_MOREFRAGS) != 0;
}

/**
 * @brief          check if IEEE80211_FCTL_RETRY is set
 *
 * @param[in]      fc    frame control bytes in little-endian byteorder
 *
 * @retval         0     no  retry
 * @retval         other has retry
 *
 * @note           None
 */
static __inline int ieee80211_has_retry(u16 fc)
{
    return (fc & IEEE80211_FCTL_RETRY) != 0;
}

/**
 * @brief          check if IEEE80211_FCTL_PM is set
 *
 * @param[in]      fc    frame control bytes in little-endian byteorder
 *
 * @retval         0     no  pm
 * @retval         other has pm
 *
 * @note           None
 */
static __inline int ieee80211_has_pm(u16 fc)
{
    return (fc & IEEE80211_FCTL_PM) != 0;
}

/**
 * @brief          check if IEEE80211_FCTL_MOREDATA is set
 *
 * @param[in]      fc    frame control bytes in little-endian byteorder
 *
 * @retval         0     no  moredata
 * @retval         other has moredata
 *
 * @note           None
 */
static __inline int ieee80211_has_moredata(u16 fc)
{
    return (fc & IEEE80211_FCTL_MOREDATA) != 0;
}

/**
 * @brief          check if IEEE80211_FCTL_PROTECTED is set
 *
 * @param[in]      fc    frame control bytes in little-endian byteorder
 *
 * @retval         0     no  protected
 * @retval         other has protected
 *
 * @note           None
 */
static __inline int ieee80211_has_protected(u16 fc)
{
    return (fc & IEEE80211_FCTL_PROTECTED) != 0;
}

/**
 * @brief          check if IEEE80211_FCTL_ORDER is set
 *
 * @param[in]      fc    frame control bytes in little-endian byteorder
 *
 * @retval         0     no  order
 * @retval         other has order
 *
 * @note           None
 */
static __inline int ieee80211_has_order(u16 fc)
{
    return (fc & IEEE80211_FCTL_ORDER) != 0;
}

/**
 * @brief          check if type is IEEE80211_FTYPE_MGMT
 *
 * @param[in]      fc    frame control bytes in little-endian byteorder
 *
 * @retval         0     not is mgmt
 * @retval         other is mgmt
 *
 * @note           None
 */
static __inline int ieee80211_is_mgmt(u16 fc)
{
    return (fc & IEEE80211_FCTL_FTYPE) == IEEE80211_FTYPE_MGMT;
}

/**
 * @brief          check if type is IEEE80211_FTYPE_CTL
 *
 * @param[in]      fc    frame control bytes in little-endian byteorder
 *
 * @retval         0     not is ctl
 * @retval         other is ctl
 *
 * @note           None
 */
static __inline int ieee80211_is_ctl(u16 fc)
{
    return (fc & IEEE80211_FCTL_FTYPE) == IEEE80211_FTYPE_CTL;
}

/**
 * @brief          check if type is IEEE80211_FTYPE_DATA
 *
 * @param[in]      fc    frame control bytes in little-endian byteorder
 *
 * @retval         0     not is data
 * @retval         other is data
 *
 * @note           None
 */
static __inline int ieee80211_is_data(u16 fc)
{
    return (fc & IEEE80211_FCTL_FTYPE) == IEEE80211_FTYPE_DATA;
}

/**
 * @brief          check if type is IEEE80211_FTYPE_DATA and IEEE80211_STYPE_QOS_DATA is set
 *
 * @param[in]      fc    frame control bytes in little-endian byteorder
 *
 * @retval         0     not is qos data
 * @retval         other is qos data
 *
 * @note           None
 */
static __inline int ieee80211_is_data_qos(u16 fc)
{
/*
 * mask with QOS_DATA rather than IEEE80211_FCTL_STYPE as we just need
 * to check the one bit
 */
    return (fc & (IEEE80211_FCTL_FTYPE | IEEE80211_STYPE_QOS_DATA)) ==
        (IEEE80211_FTYPE_DATA | IEEE80211_STYPE_QOS_DATA);
}

/**
 * @brief          check if type is IEEE80211_FTYPE_DATA and has data
 *
 * @param[in]      fc    frame control bytes in little-endian byteorder
 *
 * @retval         0     not is present data
 * @retval         other is present data
 *
 * @note           None
 */
static __inline int ieee80211_is_data_present(u16 fc)
{
/*
 * mask with 0x40 and test that that bit is clear to only return TRUE
 * for the data-containing substypes.
 */
    return (fc & (IEEE80211_FCTL_FTYPE | 0x40)) == (IEEE80211_FTYPE_DATA);
}

/**
 * @brief          check if IEEE80211_FTYPE_MGMT && IEEE80211_STYPE_ASSOC_REQ

 *
 * @param[in]      fc    frame control bytes in little-endian byteorder
 *
 * @retval         0     not is assoc req
 * @retval         other is assoc req
 *
 * @note           None
 */
static __inline int ieee80211_is_assoc_req(u16 fc)
{
    return (fc & (IEEE80211_FCTL_FTYPE | IEEE80211_FCTL_STYPE)) ==
        (IEEE80211_FTYPE_MGMT | IEEE80211_STYPE_ASSOC_REQ);
}

/**
 * @brief          check if IEEE80211_FTYPE_MGMT && IEEE80211_STYPE_ASSOC_RESP
 *
 * @param[in]      fc    frame control bytes in little-endian byteorder
 *
 * @retval         0     not is assoc resp
 * @retval         other is assoc resp
 *
 * @note           None
 */
static __inline int ieee80211_is_assoc_resp(u16 fc)
{
    return (fc & (IEEE80211_FCTL_FTYPE | IEEE80211_FCTL_STYPE)) ==
        (IEEE80211_FTYPE_MGMT | IEEE80211_STYPE_ASSOC_RESP);
}

/**
 * @brief          check if IEEE80211_FTYPE_MGMT && IEEE80211_STYPE_REASSOC_REQ
 *
 * @param[in]      fc    frame control bytes in little-endian byteorder
 *
 * @retval         0     not is reassoc req
 * @retval         other is reassoc req
 *
 * @note           None
 */
static __inline int ieee80211_is_reassoc_req(u16 fc)
{
    return (fc & (IEEE80211_FCTL_FTYPE | IEEE80211_FCTL_STYPE)) ==
        (IEEE80211_FTYPE_MGMT | IEEE80211_STYPE_REASSOC_REQ);
}

/**
 * @brief          check if IEEE80211_FTYPE_MGMT && IEEE80211_STYPE_REASSOC_RESP
 *
 * @param[in]      fc    frame control bytes in little-endian byteorder
 *
 * @retval         0     not is reassoc resp
 * @retval         other is reassoc resp
 *
 * @note           None
 */
static __inline int ieee80211_is_reassoc_resp(u16 fc)
{
    return (fc & (IEEE80211_FCTL_FTYPE | IEEE80211_FCTL_STYPE)) ==
        (IEEE80211_FTYPE_MGMT | IEEE80211_STYPE_REASSOC_RESP);
}

/**
 * @brief          check if IEEE80211_FTYPE_MGMT && IEEE80211_STYPE_PROBE_REQ
 *
 * @param[in]      fc    frame control bytes in little-endian byteorder
 *
 * @retval         0     not is probe req
 * @retval         other is probe req
 *
 * @note           None
 */
static __inline int ieee80211_is_probe_req(u16 fc)
{
    return (fc & (IEEE80211_FCTL_FTYPE | IEEE80211_FCTL_STYPE)) ==
        (IEEE80211_FTYPE_MGMT | IEEE80211_STYPE_PROBE_REQ);
}

/**
 * @brief          check if IEEE80211_FTYPE_MGMT && IEEE80211_STYPE_PROBE_RESP
 *
 * @param[in]      fc    frame control bytes in little-endian byteorder
 *
 * @retval         0     not is probe resp
 * @retval         other is probe resp
 *
 * @note           None
 */
static __inline int ieee80211_is_probe_resp(u16 fc)
{
    return (fc & (IEEE80211_FCTL_FTYPE | IEEE80211_FCTL_STYPE)) ==
        (IEEE80211_FTYPE_MGMT | IEEE80211_STYPE_PROBE_RESP);
}

/**
 * @brief          check if IEEE80211_FTYPE_MGMT && IEEE80211_STYPE_BEACON
 *
 * @param[in]      fc    frame control bytes in little-endian byteorder
 *
 * @retval         0     not is beacon
 * @retval         other is beacon
 *
 * @note           None
 */
static __inline int ieee80211_is_beacon(u16 fc)
{
    return (fc & (IEEE80211_FCTL_FTYPE | IEEE80211_FCTL_STYPE)) ==
        (IEEE80211_FTYPE_MGMT | IEEE80211_STYPE_BEACON);
}

/**
 * @brief          check if IEEE80211_FTYPE_MGMT && IEEE80211_STYPE_ATIM
 *
 * @param[in]      fc    frame control bytes in little-endian byteorder
 *
 * @retval         0     not is atim
 * @retval         other is atim
 *
 * @note           None
 */
static __inline int ieee80211_is_atim(u16 fc)
{
    return (fc & (IEEE80211_FCTL_FTYPE | IEEE80211_FCTL_STYPE)) ==
        (IEEE80211_FTYPE_MGMT | IEEE80211_STYPE_ATIM);
}

/**
 * @brief          check if IEEE80211_FTYPE_MGMT && IEEE80211_STYPE_DISASSOC
 *
 * @param[in]      fc    frame control bytes in little-endian byteorder
 *
 * @retval         0     not is disassoc
 * @retval         other is disassoc
 *
 * @note           None
 */
static __inline int ieee80211_is_disassoc(u16 fc)
{
    return (fc & (IEEE80211_FCTL_FTYPE | IEEE80211_FCTL_STYPE)) ==
        (IEEE80211_FTYPE_MGMT | IEEE80211_STYPE_DISASSOC);
}

/**
 * @brief          check if IEEE80211_FTYPE_MGMT && IEEE80211_STYPE_AUTH
 *
 * @param[in]      fc    frame control bytes in little-endian byteorder
 *
 * @retval         0     not is auth
 * @retval         other is auth
 *
 * @note           None
 */
static __inline int ieee80211_is_auth(u16 fc)
{
    return (fc & (IEEE80211_FCTL_FTYPE | IEEE80211_FCTL_STYPE)) ==
        (IEEE80211_FTYPE_MGMT | IEEE80211_STYPE_AUTH);
}

/**
 * @brief          check if IEEE80211_FTYPE_MGMT && IEEE80211_STYPE_DEAUTH
 *
 * @param[in]      fc    frame control bytes in little-endian byteorder
 *
 * @retval         0     not is deauth
 * @retval         other is deauth
 *
 * @note           None
 */
static __inline int ieee80211_is_deauth(u16 fc)
{
    return (fc & (IEEE80211_FCTL_FTYPE | IEEE80211_FCTL_STYPE)) ==
        (IEEE80211_FTYPE_MGMT | IEEE80211_STYPE_DEAUTH);
}

/**
 * @brief          check if IEEE80211_FTYPE_MGMT && IEEE80211_STYPE_ACTION
 *
 * @param[in]      fc    frame control bytes in little-endian byteorder
 *
 * @retval         0     not is action
 * @retval         other is action
 *
 * @note           None
 */
static __inline int ieee80211_is_action(u16 fc)
{
    return (fc & (IEEE80211_FCTL_FTYPE | IEEE80211_FCTL_STYPE)) ==
        (IEEE80211_FTYPE_MGMT | IEEE80211_STYPE_ACTION);
}

/**
 * @brief          check if IEEE80211_FTYPE_CTL && IEEE80211_STYPE_BACK_REQ
 *
 * @param[in]      fc    frame control bytes in little-endian byteorder
 *
 * @retval         0     not is back req
 * @retval         other is back req
 *
 * @note           None
 */
static __inline int ieee80211_is_back_req(u16 fc)
{
    return (fc & (IEEE80211_FCTL_FTYPE | IEEE80211_FCTL_STYPE)) ==
        (IEEE80211_FTYPE_CTL | IEEE80211_STYPE_BACK_REQ);
}

/**
 * @brief          check if IEEE80211_FTYPE_CTL && IEEE80211_STYPE_BACK
 *
 * @param[in]      fc    frame control bytes in little-endian byteorder
 *
 * @retval         0     not is back
 * @retval         other is back
 *
 * @note           None
 */
static __inline int ieee80211_is_back(u16 fc)
{
    return (fc & (IEEE80211_FCTL_FTYPE | IEEE80211_FCTL_STYPE)) ==
        (IEEE80211_FTYPE_CTL | IEEE80211_STYPE_BACK);
}

/**
 * @brief          check if IEEE80211_FTYPE_CTL && IEEE80211_STYPE_PSPOLL
 *
 * @param[in]      fc    frame control bytes in little-endian byteorder
 *
 * @retval         0     not is pspoll
 * @retval         other is pspoll
 *
 * @note           None
 */
static __inline int ieee80211_is_pspoll(u16 fc)
{
    return (fc & (IEEE80211_FCTL_FTYPE | IEEE80211_FCTL_STYPE)) ==
        (IEEE80211_FTYPE_CTL | IEEE80211_STYPE_PSPOLL);
}

/**
 * @brief          check if IEEE80211_FTYPE_CTL && IEEE80211_STYPE_RTS
 *
 * @param[in]      fc    frame control bytes in little-endian byteorder
 *
 * @retval         0     not is rts
 * @retval         other is rts
 *
 * @note           None
 */
static __inline int ieee80211_is_rts(u16 fc)
{
    return (fc & (IEEE80211_FCTL_FTYPE | IEEE80211_FCTL_STYPE)) ==
        (IEEE80211_FTYPE_CTL | IEEE80211_STYPE_RTS);
}

/**
 * @brief          check if IEEE80211_FTYPE_CTL && IEEE80211_STYPE_CTS
 *
 * @param[in]      fc    frame control bytes in little-endian byteorder
 *
 * @retval         0     not is cts
 * @retval         other is cts
 *
 * @note           None
 */
static __inline int ieee80211_is_cts(u16 fc)
{
    return (fc & (IEEE80211_FCTL_FTYPE | IEEE80211_FCTL_STYPE)) ==
        (IEEE80211_FTYPE_CTL | IEEE80211_STYPE_CTS);
}

/**
 * @brief          check if IEEE80211_FTYPE_CTL && IEEE80211_STYPE_ACK
 *
 * @param[in]      fc    frame control bytes in little-endian byteorder
 *
 * @retval         0     not is ack
 * @retval         other is ack
 *
 * @note           None
 */
static __inline int ieee80211_is_ack(u16 fc)
{
    return (fc & (IEEE80211_FCTL_FTYPE | IEEE80211_FCTL_STYPE)) ==
        (IEEE80211_FTYPE_CTL | IEEE80211_STYPE_ACK);
}

/**
 * @brief          check if IEEE80211_FTYPE_CTL && IEEE80211_STYPE_CFEND
 *
 * @param[in]      fc    frame control bytes in little-endian byteorder
 *
 * @retval         0     not is cfend
 * @retval         other is cfend
 *
 * @note           None
 */
static __inline int ieee80211_is_cfend(u16 fc)
{
    return (fc & (IEEE80211_FCTL_FTYPE | IEEE80211_FCTL_STYPE)) ==
        (IEEE80211_FTYPE_CTL | IEEE80211_STYPE_CFEND);
}

/**
 * @brief          check if IEEE80211_FTYPE_CTL && IEEE80211_STYPE_CFENDACK
 *
 * @param[in]      fc    frame control bytes in little-endian byteorder
 *
 * @retval         0     not is cfendack
 * @retval         other is cfendack
 *
 * @note           None
 */
static __inline int ieee80211_is_cfendack(u16 fc)
{
    return (fc & (IEEE80211_FCTL_FTYPE | IEEE80211_FCTL_STYPE)) ==
        (IEEE80211_FTYPE_CTL | IEEE80211_STYPE_CFENDACK);
}

/**
 * @brief          check if frame is a regular (non-QoS) nullfunc frame
 *
 * @param[in]      fc    frame control bytes in little-endian byteorder
 *
 * @retval         0     not is nullfunc
 * @retval         other is nullfunc
 *
 * @note           None
 */
static __inline int ieee80211_is_nullfunc(u16 fc)
{
    return (fc & (IEEE80211_FCTL_FTYPE | IEEE80211_FCTL_STYPE)) ==
        (IEEE80211_FTYPE_DATA | IEEE80211_STYPE_NULLFUNC);
}

/**
 * @brief          check if frame is a QoS nullfunc frame
 *
 * @param[in]      fc    frame control bytes in little-endian byteorder
 *
 * @retval         0     not is qos nullfunc
 * @retval         other is qos nullfunc
 *
 * @note           None
 */
static __inline int ieee80211_is_qos_nullfunc(u16 fc)
{
    return (fc & (IEEE80211_FCTL_FTYPE | IEEE80211_FCTL_STYPE)) ==
        (IEEE80211_FTYPE_DATA | IEEE80211_STYPE_QOS_NULLFUNC);
}

/**
 * struct ieee80211_quiet_ie
 *
 * This structure refers to "Quiet information element"
 */
 __packed struct ieee80211_quiet_ie {
    u8 count;
    u8 period;
    u16 duration;
    u16 offset;
};

/**
 * struct ieee80211_msrment_ie
 *
 * This structure refers to "Measurement Request/Report information element"
 */
 __packed struct ieee80211_msrment_ie {
    u8 token;
    u8 mode;
    u8 type;
    u8 request[1];
};

/**
 * struct ieee80211_channel_sw_ie
 *
 * This structure refers to "Channel Switch Announcement information element"
 */
 __packed struct ieee80211_channel_sw_ie {
    u8 mode;
    u8 new_ch_num;
    u8 count;
};

/**
 * struct ieee80211_tim
 *
 * This structure refers to "Traffic Indication Map information element"
 */
 __packed struct ieee80211_tim_ie {
    u8 dtim_count;
    u8 dtim_period;
    u8 bitmap_ctrl;
/* variable size: 1 - 251 bytes */
    u8 virtual_map[1];
};

/**
 * struct ieee80211_rann_ie
 *
 * This structure refers to "Root Announcement information element"
 */
 __packed struct ieee80211_rann_ie {
    u8 rann_flags;
    u8 rann_hopcount;
    u8 rann_ttl;
    u8 rann_addr[6];
    u32 rann_seq;
    u32 rann_metric;
};

#define WLAN_SA_QUERY_TR_ID_LEN 2

struct ieee80211_mgmt {
    u16 frame_control;
    u16 duration;
    u8 da[6];
    u8 sa[6];
    u8 bssid[6];
    u16 seq_ctrl;
	union {
		struct {
            u16 auth_alg;
            u16 auth_transaction;
            u16 status_code;
        /* possibly followed by Challenge text */
            u8 variable[0];
        } __attribute__ ((packed)) auth;
		struct {
            u16 reason_code;
        } __attribute__ ((packed)) deauth;
		struct {
            u16 capab_info;
            u16 listen_interval;
        /* followed by SSID and Supported rates */
            u8 variable[0];
        } assoc_req;
		struct {
            u16 capab_info;
            u16 status_code;
            u16 aid;
        /* followed by Supported rates */
            u8 variable[0];
        } __attribute__ ((packed)) assoc_resp, reassoc_resp;
		struct {
            u16 capab_info;
            u16 listen_interval;
            u8 current_ap[6];
        /* followed by SSID and Supported rates */
            u8 variable[0];
        } __attribute__ ((packed)) reassoc_req;
		struct {
            u16 reason_code;
        } __attribute__ ((packed)) disassoc;
		struct {
            u64 timestamp;
            u16 beacon_int;
            u16 capab_info;
        /* followed by some of SSID, Supported rates, FH Params, DS Params, CF
           Params, IBSS Params, TIM */
            u8 variable[0];
        } __attribute__ ((packed)) beacon;
		struct {
        /* only variable items: SSID, Supported rates */
            u8 variable[0];
        } __attribute__ ((packed)) probe_req;
		struct {
            u64 timestamp;
            u16 beacon_int;
            u16 capab_info;
        /* followed by some of SSID, Supported rates, FH Params, DS Params, CF
           Params, IBSS Params */
            u8 variable[0];
        } __attribute__ ((packed)) probe_resp;
		struct {
            u8 category;
			union {
				struct {
                    u8 action_code;
                    u8 dialog_token;
                    u8 status_code;
                    u8 variable[0];
                } __attribute__ ((packed)) wme_action;
				struct{
                    u8 action_code;
                    u8 element_id;
                    u8 length;
                    struct ieee80211_channel_sw_ie sw_elem;
                } __attribute__ ((packed)) chan_switch;
				struct{
                    u8 action_code;
					u8 channel_width;
				} __attribute__ ((packed)) chan_width;
				struct{
					u8 action_code;
                    u8 dialog_token;
                    u8 element_id;
                    u8 length;
                    struct ieee80211_msrment_ie msr_elem;
                } __attribute__ ((packed)) measurement;
				struct{
                    u8 action_code;
                    u8 dialog_token;
                    u16 capab;
                    u16 timeout;
                    u16 start_seq_num;
                } __attribute__ ((packed)) addba_req;
				struct{
                    u8 action_code;
                    u8 dialog_token;
                    u16 status;
                    u16 capab;
                    u16 timeout;
                } __attribute__ ((packed)) addba_resp;
				struct{
                    u8 action_code;
                    u16 params;
                    u16 reason_code;
                } __attribute__ ((packed)) delba;
				struct{
                    u8 action_code;
                /* capab_info for open and confirm, reason for close */
                    u16 aux;
                /* Followed in plink_confirm by status code, AID and supported
                   rates, and directly by supported rates in plink_open and
                   plink_close */
                    u8 variable[0];
                } __attribute__ ((packed)) plink_action;
				struct{
                    u8 action_code;
                    u8 variable[0];
                } __attribute__ ((packed)) mesh_action;
				struct {
                    u8 action;
                    u8 trans_id[WLAN_SA_QUERY_TR_ID_LEN];
                } __attribute__ ((packed)) sa_query;
				struct {
                    u8 action;
                    u8 smps_control;
                } __attribute__ ((packed)) ht_smps;
            } u;
        } __attribute__ ((packed)) action;
    } u;
} __attribute__ ((packed));

#if TLS_CONFIG_11N
/* mgmt header + 1 byte category code */
#define IEEE80211_MIN_ACTION_SIZE offsetof(struct ieee80211_mgmt, u.action.u)

#endif

/* Management MIC information element (IEEE 802.11w) */
struct ieee80211_mmie {
    u8 element_id;
    u8 length;
    u16 key_id;
    u8 sequence_number[6];
    u8 mic[8];
} __attribute__ ((packed));

/* Control frames */
struct ieee80211_rts {
    u16 frame_control;
    u16 duration;
    u8 ra[6];
    u8 ta[6];
} __attribute__ ((packed));

struct ieee80211_cts {
    u16 frame_control;
    u16 duration;
    u8 ra[6];
} __attribute__ ((packed));

 struct ieee80211_pspoll {
    u16 frame_control;
    u16 aid;
    u8 bssid[6];
    u8 ta[6];
} __attribute__ ((packed));

/**
 * struct ieee80211_bar - HT Block Ack Request
 *
 * This structure refers to "HT BlockAckReq" as
 * described in 802.11n draft section 7.2.1.7.1
 */
struct ieee80211_bar {
    u16 frame_control;
    u16 duration;
    u8 ra[6];
    u8 ta[6];
    u16 control;
    u16 start_seq_num;
} __attribute__ ((packed));

/* 802.11 BAR control masks */
#define IEEE80211_BAR_CTRL_ACK_POLICY_NORMAL      0x0000
#define IEEE80211_BAR_CTRL_CBMTID_COMPRESSED_BA   0x0004


#define IEEE80211_HT_MCS_MASK_LEN                 10

/**
 * struct ieee80211_mcs_info - MCS information
 * @rx_mask: RX mask
 * @rx_highest: highest supported RX rate. If set represents
 *	the highest supported RX data rate in units of 1 Mbps.
 *	If this field is 0 this value should not be used to
 *	consider the highest RX data rate supported.
 * @tx_params: TX parameters
 */
struct ieee80211_mcs_info {
    u8 rx_mask[IEEE80211_HT_MCS_MASK_LEN];
    u16 rx_highest;
    u8 tx_params;
    u8 reserved[3];
} __attribute__ ((packed));

/* 802.11n HT capability MSC set */
#define IEEE80211_HT_MCS_RX_HIGHEST_MASK        0x3ff
#define IEEE80211_HT_MCS_TX_DEFINED             0x01
#define IEEE80211_HT_MCS_TX_RX_DIFF             0x02
/* value 0 == 1 stream etc */
#define IEEE80211_HT_MCS_TX_MAX_STREAMS_MASK    0x0C
#define IEEE80211_HT_MCS_TX_MAX_STREAMS_SHIFT   2
#define		IEEE80211_HT_MCS_TX_MAX_STREAMS	    4
#define IEEE80211_HT_MCS_TX_UNEQUAL_MODULATION  0x10

/*
 * 802.11n D5.0 20.3.5 / 20.6 says:
 * - indices 0 to 7 and 32 are single spatial stream
 * - 8 to 31 are multiple spatial streams using equal modulation
 *   [8..15 for two streams, 16..23 for three and 24..31 for four]
 * - remainder are multiple spatial streams using unequal modulation
 */
#define IEEE80211_HT_MCS_UNEQUAL_MODULATION_START 33
#define IEEE80211_HT_MCS_UNEQUAL_MODULATION_START_BYTE \
	(IEEE80211_HT_MCS_UNEQUAL_MODULATION_START / 8)

/**
 * struct ieee80211_ht_cap - HT capabilities
 *
 * This structure is the "HT capabilities element" as
 * described in 802.11n D5.0 7.3.2.56
 */
struct ieee80211_ht_cap {
    u16 cap_info;
    u8 ampdu_params_info;

/* 16 bytes MCS information */
    struct ieee80211_mcs_info mcs;

    u16 extended_ht_cap_info;
    u32 tx_BF_cap_info;
    u8 antenna_selection_info;
} __attribute__ ((packed));

/* 802.11n HT capabilities masks (for cap_info) */
#define IEEE80211_HT_CAP_LDPC_CODING                0x0001
#define IEEE80211_HT_CAP_SUP_WIDTH_20_40            0x0002
#define IEEE80211_HT_CAP_SM_PS                      0x000C
#define		IEEE80211_HT_CAP_SM_PS_SHIFT            2
/* Spatial Multiplexing Power Save Modes (for capability) */
#define 		IEEE80211_HT_CAP_SM_PS_STATIC       0
#define 		IEEE80211_HT_CAP_SM_PS_DYNAMIC      1
#define 		IEEE80211_HT_CAP_SM_PS_INVALID      2
#define 		IEEE80211_HT_CAP_SM_PS_DISABLED     3

#define IEEE80211_HT_CAP_GRN_FLD                    0x0010
#define IEEE80211_HT_CAP_SGI_20                     0x0020
#define IEEE80211_HT_CAP_SGI_40                     0x0040
#define IEEE80211_HT_CAP_TX_STBC                    0x0080
#define IEEE80211_HT_CAP_RX_STBC                    0x0300
#define IEEE80211_HT_CAP_RX_STBC_SHIFT              8
#define IEEE80211_HT_CAP_RX_STBC_DISABLE            0
#define IEEE80211_HT_CAP_RX_STBC_1                  1
#define IEEE80211_HT_CAP_RX_STBC_2                  2
#define IEEE80211_HT_CAP_RX_STBC_3                  3

#define IEEE80211_HT_CAP_DELAY_BA                   0x0400
#define IEEE80211_HT_CAP_MAX_AMSDU                  0x0800
#define IEEE80211_HT_CAP_DSSSCCK40                  0x1000
#define IEEE80211_HT_CAP_RESERVED                   0x2000
#define IEEE80211_HT_CAP_40MHZ_INTOLERANT	        0x4000
#define IEEE80211_HT_CAP_LSIG_TXOP_PROT		        0x8000

/* 802.11n HT capability AMPDU settings (for ampdu_params_info) */
#define IEEE80211_HT_AMPDU_PARM_FACTOR              0x03
#define IEEE80211_HT_AMPDU_PARM_DENSITY             0x1C
#define		IEEE80211_HT_AMPDU_PARM_DENSITY_SHIFT   2

/*
 * Maximum length of AMPDU that the STA can receive.
 * Length = 2 ^ (13 + max_ampdu_length_exp) - 1 (octets)
 */
enum ieee80211_max_ampdu_length_exp {
    IEEE80211_HT_MAX_AMPDU_8K  = 0,
    IEEE80211_HT_MAX_AMPDU_16K = 1,
    IEEE80211_HT_MAX_AMPDU_32K = 2,
    IEEE80211_HT_MAX_AMPDU_64K = 3
};

/* 802.11n HT extended capability */
#define IEEE80211_HT_EXT_CAP_HTC        0x0400
#define IEEE80211_HT_EXT_CAP_RD         0x0800


#define IEEE80211_HT_MAX_AMPDU_FACTOR   13

/* Minimum MPDU start spacing */
enum ieee80211_min_mpdu_spacing {
    IEEE80211_HT_MPDU_DENSITY_NONE = 0, /* No restriction */
    IEEE80211_HT_MPDU_DENSITY_0_25 = 1, /* 1/4 usec */
    IEEE80211_HT_MPDU_DENSITY_0_5  = 2,  /* 1/2 usec */
    IEEE80211_HT_MPDU_DENSITY_1    = 3,    /* 1 usec */
    IEEE80211_HT_MPDU_DENSITY_2    = 4,    /* 2 usec */
    IEEE80211_HT_MPDU_DENSITY_4    = 5,    /* 4 usec */
    IEEE80211_HT_MPDU_DENSITY_8    = 6,    /* 8 usec */
    IEEE80211_HT_MPDU_DENSITY_16   = 7    /* 16 usec */
};

#define IEEE80211_HT_MPDU_DENSITY_SHIFT	2
/**
 * struct ieee80211_ht_info - HT information
 *
 * This structure is the "HT information element" as
 * described in 802.11n D5.0 7.3.2.58
 */
/* ELEMENTID = 61, HT Operation element in 11n7.3.5.57
*/

 __packed struct ieee80211_ht_info {
    u8 control_chan;
    u8 ht_param;
    u16 operation_mode;
    u16 stbc_param;
    u8 basic_set[16];
};

/* for ht_param */
#define IEEE80211_HT_PARAM_CHA_SEC_OFFSET           0x03
#define	IEEE80211_HT_PARAM_CHA_SEC_NONE             0x00
#define	IEEE80211_HT_PARAM_CHA_SEC_ABOVE            0x01
#define	IEEE80211_HT_PARAM_CHA_SEC_BELOW            0x03
#define IEEE80211_HT_PARAM_CHAN_WIDTH_ANY           0x04
#define IEEE80211_HT_PARAM_RIFS_MODE                0x08
#define IEEE80211_HT_PARAM_SPSMP_SUPPORT            0x10
#define IEEE80211_HT_PARAM_SERV_INTERVAL_GRAN       0xE0

/* for operation_mode */
#define IEEE80211_HT_OP_MODE_PROTECTION             0x0003
#define	IEEE80211_HT_OP_MODE_PROTECTION_NONE        0
#define	IEEE80211_HT_OP_MODE_PROTECTION_NONMEMBER   1
#define	IEEE80211_HT_OP_MODE_PROTECTION_20MHZ       2
#define	IEEE80211_HT_OP_MODE_PROTECTION_NONHT_MIXED 3
#define IEEE80211_HT_OP_MODE_NON_GF_STA_PRSNT       0x0004
#define IEEE80211_HT_OP_MODE_NON_HT_STA_PRSNT       0x0010

/* for stbc_param */
#define IEEE80211_HT_STBC_PARAM_DUAL_BEACON         0x0040
#define IEEE80211_HT_STBC_PARAM_DUAL_CTS_PROT       0x0080
#define IEEE80211_HT_STBC_PARAM_STBC_BEACON         0x0100
#define IEEE80211_HT_STBC_PARAM_LSIG_TXOP_FULLPROT  0x0200
#define IEEE80211_HT_STBC_PARAM_PCO_ACTIVE          0x0400
#define IEEE80211_HT_STBC_PARAM_PCO_PHASE           0x0800


/* block-ack parameters */
#define IEEE80211_ADDBA_PARAM_POLICY_MASK           0x0002
#define IEEE80211_ADDBA_PARAM_TID_MASK              0x003C
#define IEEE80211_ADDBA_PARAM_BUF_SIZE_MASK         0xFFA0
#define IEEE80211_DELBA_PARAM_TID_MASK              0xF000
#define IEEE80211_DELBA_PARAM_INITIATOR_MASK        0x0800

/*
 * A-PMDU buffer sizes
 * According to IEEE802.11n spec size varies from 8K to 64K (in powers of 2)
 */
#define IEEE80211_MIN_AMPDU_BUF                     0x8
#define IEEE80211_MAX_AMPDU_BUF                     0x40


/* Spatial Multiplexing Power Save Modes (for capability) */
#define WLAN_HT_CAP_SM_PS_STATIC                0
#define WLAN_HT_CAP_SM_PS_DYNAMIC               1
#define WLAN_HT_CAP_SM_PS_INVALID               2
#define WLAN_HT_CAP_SM_PS_DISABLED              3

/* for SM power control field lower two bits */
#define WLAN_HT_SMPS_CONTROL_DISABLED           0
#define WLAN_HT_SMPS_CONTROL_STATIC             1
#define WLAN_HT_SMPS_CONTROL_DYNAMIC            3

/* Authentication algorithms */
#define WLAN_AUTH_OPEN                          0
#define WLAN_AUTH_SHARED_KEY                    1
#define WLAN_AUTH_FT                            2
#define WLAN_AUTH_LEAP                          128

#define WLAN_AUTH_CHALLENGE_LEN                 128

#define WLAN_CAPABILITY_ESS                     (1<<0)
#define WLAN_CAPABILITY_IBSS                    (1<<1)
#define WLAN_CAPABILITY_CF_POLLABLE             (1<<2)
#define WLAN_CAPABILITY_CF_POLL_REQUEST         (1<<3)
#define WLAN_CAPABILITY_PRIVACY                 (1<<4)
#define WLAN_CAPABILITY_SHORT_PREAMBLE          (1<<5)
#define WLAN_CAPABILITY_PBCC                    (1<<6)
#define WLAN_CAPABILITY_CHANNEL_AGILITY         (1<<7)

/* 802.11h */
#define WLAN_CAPABILITY_SPECTRUM_MGMT           (1<<8)
#define WLAN_CAPABILITY_QOS                     (1<<9)
#define WLAN_CAPABILITY_SHORT_SLOT_TIME         (1<<10)
#define WLAN_CAPABILITY_DSSS_OFDM               (1<<13)
/* measurement */
#define IEEE80211_SPCT_MSR_RPRT_MODE_LATE       (1<<0)
#define IEEE80211_SPCT_MSR_RPRT_MODE_INCAPABLE  (1<<1)
#define IEEE80211_SPCT_MSR_RPRT_MODE_REFUSED    (1<<2)

#define IEEE80211_SPCT_MSR_RPRT_TYPE_BASIC	     0
#define IEEE80211_SPCT_MSR_RPRT_TYPE_CCA	     1
#define IEEE80211_SPCT_MSR_RPRT_TYPE_RPI	     2


/* 802.11g ERP information element */
#define WLAN_ERP_NON_ERP_PRESENT                (1<<0)
#define WLAN_ERP_USE_PROTECTION                 (1<<1)
#define WLAN_ERP_BARKER_PREAMBLE                (1<<2)

/* WLAN_ERP_BARKER_PREAMBLE values */
enum {
    WLAN_ERP_PREAMBLE_SHORT = 0,
    WLAN_ERP_PREAMBLE_LONG  = 1,
};

/* Status codes */
enum ieee80211_statuscode {
    WLAN_STATUS_SUCCESS                            = 0,
    WLAN_STATUS_UNSPECIFIED_FAILURE                = 1,
    WLAN_STATUS_CAPS_UNSUPPORTED                   = 10,
    WLAN_STATUS_REASSOC_NO_ASSOC                   = 11,
    WLAN_STATUS_ASSOC_DENIED_UNSPEC                = 12,
    WLAN_STATUS_NOT_SUPPORTED_AUTH_ALG             = 13,
    WLAN_STATUS_UNKNOWN_AUTH_TRANSACTION           = 14,
    WLAN_STATUS_CHALLENGE_FAIL                     = 15,
    WLAN_STATUS_AUTH_TIMEOUT                       = 16,
    WLAN_STATUS_AP_UNABLE_TO_HANDLE_NEW_STA        = 17,
    WLAN_STATUS_ASSOC_DENIED_RATES                 = 18,
/* 802.11b */
    WLAN_STATUS_ASSOC_DENIED_NOSHORTPREAMBLE       = 19,
    WLAN_STATUS_ASSOC_DENIED_NOPBCC                = 20,
    WLAN_STATUS_ASSOC_DENIED_NOAGILITY             = 21,
/* 802.11h */
    WLAN_STATUS_ASSOC_DENIED_NOSPECTRUM            = 22,
    WLAN_STATUS_ASSOC_REJECTED_BAD_POWER           = 23,
    WLAN_STATUS_ASSOC_REJECTED_BAD_SUPP_CHAN       = 24,
/* 802.11g */
    WLAN_STATUS_ASSOC_DENIED_NOSHORTTIME           = 25,
    WLAN_STATUS_ASSOC_DENIED_NODSSSOFDM            = 26,

    WLAN_STATUS_ASSOC_DENIED_NO_HT                 = 27,
/* 802.11w */
    WLAN_STATUS_ASSOC_REJECTED_TEMPORARILY         = 30,
    WLAN_STATUS_ROBUST_MGMT_FRAME_POLICY_VIOLATION = 31,
/* 802.11i */
    WLAN_STATUS_INVALID_IE                         = 40,
    WLAN_STATUS_INVALID_GROUP_CIPHER               = 41,
    WLAN_STATUS_INVALID_PAIRWISE_CIPHER            = 42,
    WLAN_STATUS_INVALID_AKMP                       = 43,
    WLAN_STATUS_UNSUPP_RSN_VERSION                 = 44,
    WLAN_STATUS_INVALID_RSN_IE_CAP                 = 45,
    WLAN_STATUS_CIPHER_SUITE_REJECTED              = 46,
/* 802.11e */
    WLAN_STATUS_UNSPECIFIED_QOS                    = 32,
    WLAN_STATUS_ASSOC_DENIED_NOBANDWIDTH           = 33,
    WLAN_STATUS_ASSOC_DENIED_LOWACK                = 34,
    WLAN_STATUS_ASSOC_DENIED_UNSUPP_QOS            = 35,
    WLAN_STATUS_REQUEST_DECLINED                   = 37,
    WLAN_STATUS_INVALID_QOS_PARAM                  = 38,
    WLAN_STATUS_CHANGE_TSPEC                       = 39,
    WLAN_STATUS_WAIT_TS_DELAY                      = 47,
    WLAN_STATUS_NO_DIRECT_LINK                     = 48,
    WLAN_STATUS_STA_NOT_PRESENT                    = 49,
    WLAN_STATUS_STA_NOT_QSTA                       = 50,
    WLAN_STATUS_ASSOC_DENIED_LISTEN_INT_TOO_LARGE  = 51,
};


/* Reason codes */
enum ieee80211_reasoncode {
    WLAN_REASON_UNSPECIFIED                    = 1,
    WLAN_REASON_PREV_AUTH_NOT_VALID            = 2,
    WLAN_REASON_DEAUTH_LEAVING                 = 3,
    WLAN_REASON_DISASSOC_DUE_TO_INACTIVITY     = 4,
    WLAN_REASON_DISASSOC_AP_BUSY               = 5,
    WLAN_REASON_CLASS2_FRAME_FROM_NONAUTH_STA  = 6,
    WLAN_REASON_CLASS3_FRAME_FROM_NONASSOC_STA = 7,
    WLAN_REASON_DISASSOC_STA_HAS_LEFT          = 8,
    WLAN_REASON_STA_REQ_ASSOC_WITHOUT_AUTH     = 9,
/* 802.11h */
    WLAN_REASON_DISASSOC_BAD_POWER             = 10,
    WLAN_REASON_DISASSOC_BAD_SUPP_CHAN         = 11,
/* 802.11i */
    WLAN_REASON_INVALID_IE                     = 13,
    WLAN_REASON_MIC_FAILURE                    = 14,
    WLAN_REASON_4WAY_HANDSHAKE_TIMEOUT         = 15,
    WLAN_REASON_GROUP_KEY_HANDSHAKE_TIMEOUT    = 16,
    WLAN_REASON_IE_DIFFERENT                   = 17,
    WLAN_REASON_INVALID_GROUP_CIPHER           = 18,
    WLAN_REASON_INVALID_PAIRWISE_CIPHER        = 19,
    WLAN_REASON_INVALID_AKMP                   = 20,
    WLAN_REASON_UNSUPP_RSN_VERSION             = 21,
    WLAN_REASON_INVALID_RSN_IE_CAP             = 22,
    WLAN_REASON_IEEE8021X_FAILED               = 23,
    WLAN_REASON_CIPHER_SUITE_REJECTED          = 24,
/* 802.11e */
    WLAN_REASON_DISASSOC_UNSPECIFIED_QOS       = 32,
    WLAN_REASON_DISASSOC_QAP_NO_BANDWIDTH      = 33,
    WLAN_REASON_DISASSOC_LOW_ACK               = 34,
    WLAN_REASON_DISASSOC_QAP_EXCEED_TXOP       = 35,
    WLAN_REASON_QSTA_LEAVE_QBSS                = 36,
    WLAN_REASON_QSTA_NOT_USE                   = 37,
    WLAN_REASON_QSTA_REQUIRE_SETUP             = 38,
    WLAN_REASON_QSTA_TIMEOUT                   = 39,
    WLAN_REASON_QSTA_CIPHER_NOT_SUPP           = 45,
};


/* Information Element IDs */
enum ieee80211_eid {
    WLAN_EID_SSID                         = 0,
    WLAN_EID_SUPP_RATES                   = 1,
    WLAN_EID_FH_PARAMS                    = 2,
    WLAN_EID_DS_PARAMS                    = 3,
    WLAN_EID_CF_PARAMS                    = 4,
    WLAN_EID_TIM                          = 5,
    WLAN_EID_IBSS_PARAMS                  = 6,
    WLAN_EID_CHALLENGE                    = 16,

    WLAN_EID_COUNTRY                      = 7,
    WLAN_EID_HP_PARAMS                    = 8,
    WLAN_EID_HP_TABLE                     = 9,
    WLAN_EID_REQUEST                      = 10,

    WLAN_EID_QBSS_LOAD                    = 11,
    WLAN_EID_EDCA_PARAM_SET               = 12,
    WLAN_EID_TSPEC                        = 13,
    WLAN_EID_TCLAS                        = 14,
    WLAN_EID_SCHEDULE                     = 15,
    WLAN_EID_TS_DELAY                     = 43,
    WLAN_EID_TCLAS_PROCESSING             = 44,
    WLAN_EID_QOS_CAPA                     = 46,
/* 802.11s */
    WLAN_EID_MESH_CONFIG                  = 113,
    WLAN_EID_MESH_ID                      = 114,
    WLAN_EID_LINK_METRIC_REPORT           = 115,
    WLAN_EID_CONGESTION_NOTIFICATION      = 116,
/* Note that the Peer Link IE has been replaced with the similar Peer
   Management IE.  We will keep the former definition until mesh code is
   changed to comply with latest 802.11s drafts. */
    WLAN_EID_PEER_LINK                    = 55,    /* no longer in 802.11s drafts */
    WLAN_EID_PEER_MGMT                    = 117,
    WLAN_EID_CHAN_SWITCH_PARAM            = 118,
    WLAN_EID_MESH_AWAKE_WINDOW            = 119,
    WLAN_EID_BEACON_TIMING                = 120,
    WLAN_EID_MCCAOP_SETUP_REQ             = 121,
    WLAN_EID_MCCAOP_SETUP_RESP            = 122,
    WLAN_EID_MCCAOP_ADVERT                = 123,
    WLAN_EID_MCCAOP_TEARDOWN              = 124,
    WLAN_EID_GANN                         = 125,
    WLAN_EID_RANN                         = 126,
    WLAN_EID_PREQ                         = 130,
    WLAN_EID_PREP                         = 131,
    WLAN_EID_PERR                         = 132,
    WLAN_EID_PXU                          = 137,
    WLAN_EID_PXUC                         = 138,
    WLAN_EID_AUTH_MESH_PEER_EXCH          = 139,
    WLAN_EID_MIC                          = 140,

    WLAN_EID_PWR_CONSTRAINT               = 32,
    WLAN_EID_PWR_CAPABILITY               = 33,
    WLAN_EID_TPC_REQUEST                  = 34,
    WLAN_EID_TPC_REPORT                   = 35,
    WLAN_EID_SUPPORTED_CHANNELS           = 36,
    WLAN_EID_CHANNEL_SWITCH               = 37,
    WLAN_EID_MEASURE_REQUEST              = 38,
    WLAN_EID_MEASURE_REPORT               = 39,
    WLAN_EID_QUIET                        = 40,
    WLAN_EID_IBSS_DFS                     = 41,

    WLAN_EID_ERP_INFO                     = 42,
    WLAN_EID_EXT_SUPP_RATES               = 50,

    WLAN_EID_HT_CAPABILITY                = 45,
    WLAN_EID_HT_INFORMATION               = 61,

    WLAN_EID_RSN                          = 48,
    WLAN_EID_MMIE                         = 76,
    WLAN_EID_WPA                          = 221,
    WLAN_EID_GENERIC                      = 221,
    WLAN_EID_VENDOR_SPECIFIC              = 221,
    WLAN_EID_QOS_PARAMETER                = 222,

    WLAN_EID_AP_CHAN_REPORT               = 51,
    WLAN_EID_NEIGHBOR_REPORT              = 52,
    WLAN_EID_RCPI                         = 53,
    WLAN_EID_BSS_AVG_ACCESS_DELAY         = 63,
    WLAN_EID_ANTENNA_INFO                 = 64,
    WLAN_EID_RSNI                         = 65,
    WLAN_EID_MEASUREMENT_PILOT_TX_INFO    = 66,
    WLAN_EID_BSS_AVAILABLE_CAPACITY       = 67,
    WLAN_EID_BSS_AC_ACCESS_DELAY          = 68,
    WLAN_EID_RRM_ENABLED_CAPABILITIES     = 70,
    WLAN_EID_MULTIPLE_BSSID               = 71,
    WLAN_EID_BSS_COEX_2040                = 72,
    WLAN_EID_OVERLAP_BSS_SCAN_PARAM       = 74,
    WLAN_EID_EXT_CAPABILITY               = 127,

    WLAN_EID_MOBILITY_DOMAIN              = 54,
    WLAN_EID_FAST_BSS_TRANSITION          = 55,
    WLAN_EID_TIMEOUT_INTERVAL             = 56,
    WLAN_EID_RIC_DATA                     = 57,
    WLAN_EID_RIC_DESCRIPTOR               = 75,

    WLAN_EID_DSE_REGISTERED_LOCATION      = 58,
    WLAN_EID_SUPPORTED_REGULATORY_CLASSES = 59,
    WLAN_EID_EXT_CHANSWITCH_ANN           = 60,

    WLAN_EID_TIME_ZONE                    = 98,
    WLAN_EID_LINK_ID                      = 101,
    WLAN_EID_INTERWORKING                 = 107,
    WLAN_EID_ADV_PROTO                    = 108,
    WLAN_EID_ROAMING_CONSORTIUM           = 111,
    WLAN_EID_EXT_CAPAB                    = 127,

};

/* Action category code */
enum ieee80211_category {
    WLAN_CATEGORY_SPECTRUM_MGMT             = 0,
    WLAN_CATEGORY_QOS                       = 1,
    WLAN_CATEGORY_DLS                       = 2,
    WLAN_CATEGORY_BACK                      = 3,
    WLAN_CATEGORY_PUBLIC                    = 4,
    WLAN_CATEGORY_HT                        = 7,
    WLAN_CATEGORY_SA_QUERY                  = 8,
    WLAN_CATEGORY_PROTECTED_DUAL_OF_ACTION  = 9,
    WLAN_CATEGORY_WMM                       = 17,
    WLAN_CATEGORY_VENDOR_SPECIFIC_PROTECTED = 126,
    WLAN_CATEGORY_VENDOR_SPECIFIC           = 127,
};

/* SPECTRUM_MGMT action code */
enum ieee80211_spectrum_mgmt_actioncode {
    WLAN_ACTION_SPCT_MSR_REQ    = 0,
    WLAN_ACTION_SPCT_MSR_RPRT   = 1,
    WLAN_ACTION_SPCT_TPC_REQ    = 2,
    WLAN_ACTION_SPCT_TPC_RPRT   = 3,
    WLAN_ACTION_SPCT_CHL_SWITCH = 4,
};

/* HT action codes */
enum ieee80211_ht_actioncode {
    WLAN_HT_ACTION_NOTIFY_CHANWIDTH  = 0,
    WLAN_HT_ACTION_SMPS              = 1,
    WLAN_HT_ACTION_PSMP              = 2,
    WLAN_HT_ACTION_PCO_PHASE         = 3,
    WLAN_HT_ACTION_CSI               = 4,
    WLAN_HT_ACTION_NONCOMPRESSED_BF  = 5,
    WLAN_HT_ACTION_COMPRESSED_BF     = 6,
    WLAN_HT_ACTION_ASEL_IDX_FEEDBACK = 7,
};

/* Security key length */
enum ieee80211_key_len {
    WLAN_KEY_LEN_WEP40    = 5,
    WLAN_KEY_LEN_WEP104   = 13,
    WLAN_KEY_LEN_CCMP     = 16,
    WLAN_KEY_LEN_TKIP     = 32,
    WLAN_KEY_LEN_AES_CMAC = 16,
};

/*
 * IEEE 802.11-2007 7.3.2.9 Country information element
 *
 * Minimum length is 8 octets, ie len must be evenly
 * divisible by 2
 */

/* Although the spec says 8 I'm seeing 6 in practice */
#define IEEE80211_COUNTRY_IE_MIN_LEN	6

/*
 * For regulatory extension stuff see IEEE 802.11-2007
 * Annex I (page 1141) and Annex J (page 1147). Also
 * review 7.3.2.9.
 *
 * When dot11RegulatoryClassesRequired is TRUE and the
 * first_channel/reg_extension_id is >= 201 then the IE
 * compromises of the 'ext' struct represented below:
 *
 *  - Regulatory extension ID - when generating IE this just needs
 *    to be monotonically increasing for each triplet passed in
 *    the IE
 *  - Regulatory class - index into set of rules
 *  - Coverage class - index into air propagation time (Table 7-27),
 *    in microseconds, you can compute the air propagation time from
 *    the index by multiplying by 3, so index 10 yields a propagation
 *    of 10 us. Valid values are 0-31, values 32-255 are not defined
 *    yet. A value of 0 inicates air propagation of <= 1 us.
 *
 *  See also Table I.2 for Emission limit sets and table
 *  I.3 for Behavior limit sets. Table J.1 indicates how to map
 *  a reg_class to an emission limit set and behavior limit set.
 */
#define IEEE80211_COUNTRY_EXTENSION_ID 201

/*
 *  Channels numbers in the IE must be monotonically increasing
 *  if dot11RegulatoryClassesRequired is not TRUE.
 *
 *  If dot11RegulatoryClassesRequired is TRUE consecutive
 *  subband triplets following a regulatory triplet shall
 *  have monotonically increasing first_channel number fields.
 *
 *  Channel numbers shall not overlap.
 *
 *  Note that max_power is signed.
 */
 __packed struct ieee80211_country_ie_triplet {
	 __packed union {
		 __packed struct {
            u8 first_channel;
            u8 num_channels;
            s8 max_power;
        } chans;
		 __packed struct {
            u8 reg_extension_id;
            u8 reg_class;
            u8 coverage_class;
        } ext;
    } u;
};

enum ieee80211_timeout_interval_type {
    WLAN_TIMEOUT_REASSOC_DEADLINE = 1 /* 802.11r */ ,
    WLAN_TIMEOUT_KEY_LIFETIME     = 2 /* 802.11r */ ,
    WLAN_TIMEOUT_ASSOC_COMEBACK   = 3 /* 802.11w */ ,
};

/* BACK action code */
enum ieee80211_back_actioncode {
    WLAN_ACTION_ADDBA_REQ  = 0,
    WLAN_ACTION_ADDBA_RESP = 1,
    WLAN_ACTION_DELBA      = 2,
};

/* BACK (block-ack) parties */
enum ieee80211_back_parties {
    WLAN_BACK_RECIPIENT = 0,
    WLAN_BACK_INITIATOR = 1,
    WLAN_BACK_TIMER     = 2,
};

/* SA Query action */
enum ieee80211_sa_query_action {
    WLAN_ACTION_SA_QUERY_REQUEST  = 0,
    WLAN_ACTION_SA_QUERY_RESPONSE = 1,
};


/* A-MSDU 802.11n */
#define IEEE80211_QOS_CONTROL_A_MSDU_PRESENT    0x0080

/* cipher suite selectors */
#define WLAN_CIPHER_SUITE_USE_GROUP	            0x000FAC00
#define WLAN_CIPHER_SUITE_WEP40                 0x000FAC01
#define WLAN_CIPHER_SUITE_TKIP                  0x000FAC02
/* reserved: 				0x000FAC03 */
#define WLAN_CIPHER_SUITE_CCMP                  0x000FAC04
#define WLAN_CIPHER_SUITE_WEP104                0x000FAC05
#define WLAN_CIPHER_SUITE_AES_CMAC              0x000FAC06

/* AKM suite selectors */
#define WLAN_AKM_SUITE_8021X                    0x000FAC01
#define WLAN_AKM_SUITE_PSK                      0x000FAC02

#define WLAN_MAX_KEY_LEN                        32

#define WLAN_PMKID_LEN                          16

/**
 * @brief          get pointer to qos control bytes
 *
 * @param[in]      *hdr    the frame
 *
 * @return         pointer to qos control bytes
 *
 * @note
 * The qos ctrl bytes come after the frame_control, duration, seq_num
 * and 3 or 4 addresses of length ETH_ALEN.
 * 3 addr: 2 + 2 + 2 + 3*6 = 24
 * 4 addr: 2 + 2 + 2 + 4*6 = 30
 */
static __inline u8 *ieee80211_get_qos_ctl(struct ieee80211_hdr *hdr)
{
    if (ieee80211_has_a4(hdr->frame_control))
        return (u8 *) hdr + 30;
    else
        return (u8 *) hdr + 24;
}

/**
 * @brief          get pointer to SA
 *
 * @param[in]      *hdr    the frame
 *
 * @return         SA address
 *
 * @note
 * Given an 802.11 frame, this function returns the offset
 * to the source address (SA). It does not verify that the
 * header is long enough to contain the address, and the
 * header must be long enough to contain the frame control
 * field.
 */
static __inline u8 *ieee80211_get_SA(struct ieee80211_hdr *hdr)
{
    if (ieee80211_has_a4(hdr->frame_control))
        return hdr->addr4;
    if (ieee80211_has_fromds(hdr->frame_control))
        return hdr->addr3;
    return hdr->addr2;
}

/**
 * @brief          get pointer to DA
 *
 * @param[in]      *hdr    the frame
 *
 * @return         DA address
 *
 * @note
 * Given an 802.11 frame, this function returns the offset
 * to the destination address (DA). It does not verify that
 * the header is long enough to contain the address, and the
 * header must be long enough to contain the frame control
 * field.
 */
static __inline u8 *ieee80211_get_DA(struct ieee80211_hdr *hdr)
{
    if (ieee80211_has_tods(hdr->frame_control))
        return hdr->addr3;
    else
        return hdr->addr1;
}

/**
 * @brief          check if frame is a robust management frame
 *
 * @param[in]      *hdr    the frame (buffer must include at least the first octet of payload)
 *
 * @retval         TRUE    yes
 * @retval         FALSE   no
 *
 * @note           None
 */
static __inline bool ieee80211_is_robust_mgmt_frame(struct ieee80211_hdr *hdr)
{
    if (ieee80211_is_disassoc(hdr->frame_control) ||
        ieee80211_is_deauth(hdr->frame_control))
        return TRUE;

	if (ieee80211_is_action(hdr->frame_control)) {
        u8 *category;

    /*
     * Action frames, excluding Public Action frames, are Robust
     * Management Frames. However, if we are looking at a Protected
     * frame, skip the check since the data may be encrypted and
     * the frame has already been found to be a Robust Management
     * Frame (by the other end).
     */
        if (ieee80211_has_protected(hdr->frame_control))
            return TRUE;
        category = ((u8 *) hdr) + 24;
        return *category != WLAN_CATEGORY_PUBLIC &&
            *category != WLAN_CATEGORY_HT &&
            *category != WLAN_CATEGORY_VENDOR_SPECIFIC;
    }

    return FALSE;
}

/**
 * @brief          get channel frequency
 *
 * @param[in]      channel    the FHSS channel
 *
 * @retval         -1         failed
 * @retval         other      frequency
 *
 * @note
 * Convert IEEE802.11 FHSS channel to frequency (MHz)
 * Ref IEEE 802.11-2007 section 14.6
 */
static __inline int ieee80211_fhss_chan_to_freq(int channel)
{
    if ((channel > 1) && (channel < 96))
        return channel + 2400;
    else
        return -1;
}

/**
 * @brief          get channel
 *
 * @param[in]      freq    the channels frequency
 *
 * @retval         -1      failed
 * @retval         other   channel
 *
 * @note
 * Convert frequency (MHz) to IEEE802.11 FHSS channel
 * Ref IEEE 802.11-2007 section 14.6
 */
static __inline int ieee80211_freq_to_fhss_chan(int freq)
{
    if ((freq > 2401) && (freq < 2496))
        return freq - 2400;
    else
        return -1;
}

/**
 * @brief          get channel center frequency
 *
 * @param[in]      channel    the DSSS channel
 *
 * @retval         -1         failed
 * @retval         other      frequency
 *
 * @note
 * Convert IEEE802.11 DSSS channel to the center frequency (MHz).
 * Ref IEEE 802.11-2007 section 15.6
 */
static __inline int ieee80211_dsss_chan_to_freq(int channel)
{
    if ((channel > 0) && (channel < 14))
        return 2407 + (channel * 5);
    else if (channel == 14)
        return 2484;
    else
        return -1;
}

/**
 * @brief          get channel
 *
 * @param[in]      freq    the frequency
 *
 * @retval         -1      failed
 * @retval         other   channel
 *
 * @note
 * Convert frequency (MHz) to IEEE802.11 DSSS channel
 * Ref IEEE 802.11-2007 section 15.6
 * This routine selects the channel with the closest center frequency.
 */
static __inline int ieee80211_freq_to_dsss_chan(int freq)
{
    if ((freq >= 2410) && (freq < 2475))
        return (freq - 2405) / 5;
    else if ((freq >= 2482) && (freq < 2487))
        return 14;
    else
        return -1;
}

/** Convert IEEE802.11 HR DSSS channel to frequency (MHz). Ref IEEE 802.11-2007 section 18.4.6.2.
    The channels and frequencies are the same as those defined for DSSS. */
#define ieee80211_hr_chan_to_freq(chan) ieee80211_dsss_chan_to_freq(chan)
/** Convert IEEE802.11 frequency (MHz) to HR DSSS channel. Ref IEEE 802.11-2007 section 18.4.6.2.
    The channels and frequencies are the same as those defined for DSSS. */
#define ieee80211_freq_to_hr_chan(freq) ieee80211_freq_to_dsss_chan(freq)

/** Convert IEEE802.11 ERP channel to frequency (MHz). Ref IEEE 802.11-2007 section 19.4.2 */
#define ieee80211_erp_chan_to_freq(chan) ieee80211_hr_chan_to_freq(chan)
/** Convert IEEE802.11 frequency (MHz) to ERP channel. Ref IEEE 802.11-2007 section 19.4.2 */
#define ieee80211_freq_to_erp_chan(freq) ieee80211_freq_to_hr_chan(freq)

/**
 * @brief          get channel center frequency
 *
 * @param[in]      s_freq     starting frequency == (dotChannelStartingFactor/2) MHz
 * @param[in]      channel    the OFDM channel
 *
 * @retval         -1         failed
 * @retval         other      frequency
 *
 * @note
 * Convert IEEE802.11 OFDM channel to center frequency (MHz)
 * Ref IEEE 802.11-2007 section 17.3.8.3.2
 */
static __inline int ieee80211_ofdm_chan_to_freq(int s_freq, int channel)
{
    if ((channel > 0) && (channel <= 200) && (s_freq >= 4000))
        return s_freq + (channel * 5);
    else
        return -1;
}

/**
 * @brief          get channel
 *
 * @param[in]      s_freq    starting frequency == (dotChannelStartingFactor/2) MHz
 * @param[in]      freq      the frequency
 *
 * @retval         -1        failed
 * @retval         other     channel
 *
 * @note
 * Convert frequency (MHz) to IEEE802.11 OFDM channel
 * Ref IEEE 802.11-2007 section 17.3.8.3.2
 * This routine selects the channel with the closest center frequency.
 */
static __inline int ieee80211_freq_to_ofdm_chan(int s_freq, int freq)
{
    if ((freq > (s_freq + 2)) && (freq <= (s_freq + 1202)) && (s_freq >= 4000))
        return (freq + 2 - s_freq) / 5;
    else
        return -1;
}

/**
 * @brief          convert time units (TU) to microseconds
 *
 * @param[in]      tu    the TUs
 *
 * @return         microseconds value
 *
 * @note           None
 */
static __inline unsigned long ieee80211_tu_to_usec(unsigned long tu)
{
    return 1024 * tu;
}

/**
 * @brief          check if AID bit is set in TIM
 *
 * @param[in]      *tim       the TIM IE
 * @param[in]      tim_len    length of the TIM IE
 * @param[in]      aid        the AID to look for
 *
 * @retval         0          AID bit not is set in TIM
 * @retval         other      AID bit is set im TIM
 *
 * @note           None
 */
static __inline bool ieee80211_check_tim(struct ieee80211_tim_ie *tim,
                                         u8 tim_len, u16 aid)
{
    u8 mask;
    u8 index, indexn1, indexn2;

    if (!tim || tim_len < sizeof(*tim))
        return FALSE;

    aid &= 0x3fff;
    index = aid / 8;
    mask = 1 << (aid & 7);

    indexn1 = tim->bitmap_ctrl & 0xfe;
    indexn2 = tim_len + indexn1 - 4;

    if (index < indexn1 || index > indexn2)
        return FALSE;

    index -= indexn1;

    return !!(tim->virtual_map[index] & mask);
}

#endif /* end of TLS_IEEE80211_H */
