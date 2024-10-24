/**************************************************************************
 * File Name                   : tls_cmdp_ri.c
 * Author                      :
 * Version                     :
 * Date                        :
 * Description                 :
 *
 * Copyright (c) 2014 Winner Microelectronics Co., Ltd. 
 * All rights reserved.
 *
 ***************************************************************************/
#include "wm_cmdp_ri.h"
#if (GCC_COMPILE==1)
#include "wm_cmdp_hostif_gcc.h"
#else
#include "wm_cmdp_hostif.h"
#endif
#include "wm_cmdp.h"
#include "wm_debug.h"
#include "wm_mem.h"
#include <string.h>
#if 0
#if TLS_CONFIG_HOSTIF

#if TLS_CONFIG_RI_CMD

int ricmd_nop_proc(char *buf, u32 length, 
        char *cmdrsp_buf, u32 *cmdrsp_size)
{
    u8 err = 0;
    struct tls_hostif_cmdrsp *cmdrsp = (struct tls_hostif_cmdrsp *)cmdrsp_buf;

    if (length != sizeof(struct tls_hostif_cmd_hdr))
        err = CMD_ERR_INV_PARAMS;
    
    tls_hostif_fill_cmdrsp_hdr(cmdrsp, HOSTIF_CMD_NOP, err, 0);
    *cmdrsp_size = sizeof(struct tls_hostif_cmd_hdr);
    
    return 0;

} 


int ricmd_reset_proc(char *buf, u32 length, 
        char *cmdrsp_buf, u32 *cmdrsp_size)
{
    u8 err = 0; 
    struct tls_hostif_cmdrsp *cmdrsp = (struct tls_hostif_cmdrsp *)cmdrsp_buf;

    if (length != sizeof(struct tls_hostif_cmd_hdr))
        err = CMD_ERR_INV_PARAMS;

    tls_hostif_fill_cmdrsp_hdr(cmdrsp, HOSTIF_CMD_RESET, err, 0);
    *cmdrsp_size = sizeof(struct tls_hostif_cmd_hdr);
    tls_cmd_reset_sys();

    return 0;
}

int ricmd_ps_proc(char *buf, u32 length, 
        char *cmdrsp_buf, u32 *cmdrsp_size)
{
    struct tls_hostif_cmdrsp *cmdrsp = (struct tls_hostif_cmdrsp *)cmdrsp_buf;
    struct tls_hostif_cmd *cmd = (struct tls_hostif_cmd *)buf;
    struct tls_cmd_ps_t ps;
    u8 err = 0; 

    if (length != sizeof(struct tls_hostif_cmd_hdr)+6)
        err = CMD_ERR_INV_PARAMS;

    if (!err) {
        ps.ps_type = cmd->params.ps.ps_type;
        ps.wake_type = cmd->params.ps.wake_type;
        ps.delay_time = get_unaligned_be16((u8 *)&cmd->params.ps.delay_time);
        ps.wake_time = get_unaligned_be16((u8 *)&cmd->params.ps.wake_time);

        err = tls_cmd_ps(&ps); 
    } 

    tls_hostif_fill_cmdrsp_hdr(cmdrsp, HOSTIF_CMD_PS, err, 0);
    *cmdrsp_size = sizeof(struct tls_hostif_cmd_hdr);

    return 0; 
}

int ricmd_reset_flash_proc(char *buf, u32 length, 
        char *cmdrsp_buf, u32 *cmdrsp_size)
{
    struct tls_hostif_cmdrsp *cmdrsp = (struct tls_hostif_cmdrsp *)cmdrsp_buf;
    int err = 0; 

    do {
        if (length != sizeof(struct tls_hostif_cmd_hdr)) {
            err = CMD_ERR_INV_PARAMS;
            break;
        }

        err = tls_cmd_reset_flash();
        if (err)
            err = CMD_ERR_OPS;
    } while(0);

    tls_hostif_fill_cmdrsp_hdr(cmdrsp, HOSTIF_CMD_RESET_FLASH, (u8)err, 0);
    *cmdrsp_size = sizeof(struct tls_hostif_cmd_hdr); 

    return 0; 
}

int ricmd_pmtf_proc(char *buf, u32 length, 
        char *cmdrsp_buf, u32 *cmdrsp_size)
{
    struct tls_hostif_cmdrsp *cmdrsp = (struct tls_hostif_cmdrsp *)cmdrsp_buf;
    int err = 0; 

    do {
        if (length != sizeof(struct tls_hostif_cmd_hdr)) {
            err = CMD_ERR_INV_PARAMS;
            break;
        }

        err = tls_cmd_pmtf();
        if (err)
            err = CMD_ERR_OPS;
    } while(0);

    tls_hostif_fill_cmdrsp_hdr(cmdrsp, HOSTIF_CMD_PMTF, (u8)err, 0);
    *cmdrsp_size = sizeof(struct tls_hostif_cmd_hdr); 

    return 0;
}

int ricmd_gpio_proc(char *buf, u32 length, 
        char *cmdrsp_buf, u32 *cmdrsp_size)
{
    struct tls_hostif_cmdrsp *cmdrsp = (struct tls_hostif_cmdrsp *)cmdrsp_buf;

    tls_hostif_fill_cmdrsp_hdr(cmdrsp, HOSTIF_CMD_GPIO, 0, 0);
    *cmdrsp_size = sizeof(struct tls_hostif_cmd_hdr);
    /* TODO */

    return 0;

}

int ricmd_get_mac_proc(char *buf, u32 length, 
        char *cmdrsp_buf, u32 *cmdrsp_size)
{
    struct tls_hostif_cmdrsp *cmdrsp = (struct tls_hostif_cmdrsp *)cmdrsp_buf;
    u8 *mac_addr;
    u8 err = 0; 

    do {
        if (length != sizeof(struct tls_hostif_cmd_hdr)) {
            err = CMD_ERR_INV_PARAMS; 
            break;
        }

        mac_addr = (u8* )wpa_supplicant_get_mac();
        if (!mac_addr) {
            err = CMD_ERR_OPS; 
        } else {
            MEMCPY(&cmdrsp->params.mac.addr, mac_addr, ETH_ALEN);
        }
    } while(0);

    if (err) {
        tls_hostif_fill_cmdrsp_hdr(cmdrsp, HOSTIF_CMD_MAC, err, 0);
        *cmdrsp_size = sizeof(struct tls_hostif_cmd_hdr);
    }
    else {
        tls_hostif_fill_cmdrsp_hdr(cmdrsp, HOSTIF_CMD_MAC, 0, 1);
        *cmdrsp_size = sizeof(struct tls_hostif_cmd_hdr) + ETH_ALEN;
    } 

    return 0; 
}

int ricmd_ver_proc(char *buf, u32 length, 
        char *cmdrsp_buf, u32 *cmdrsp_size)
{
    struct tls_hostif_cmdrsp *cmdrsp = (struct tls_hostif_cmdrsp *)cmdrsp_buf;
    struct tls_cmd_ver_t ver;
    u8 err = 0; 

    do {
        if (length != sizeof(struct tls_hostif_cmd_hdr)) {
            err = CMD_ERR_INV_PARAMS; 
            break;
        }

        tls_cmd_get_ver(&ver);
        MEMCPY(&cmdrsp->params.ver.hw_ver, ver.hw_ver, 6);
        MEMCPY(&cmdrsp->params.ver.fw_ver, ver.fw_ver, 4);

    } while(0);

    if (err) {
        tls_hostif_fill_cmdrsp_hdr(cmdrsp, HOSTIF_CMD_VER, err, 0);
        *cmdrsp_size = sizeof(struct tls_hostif_cmd_hdr);
    }
    else {
        tls_hostif_fill_cmdrsp_hdr(cmdrsp, HOSTIF_CMD_VER, 0, 1);
        *cmdrsp_size = 10 + sizeof(struct tls_hostif_cmd_hdr); 
    } 

    return 0; 
}

int ricmd_join_proc(char *buf, u32 length, 
        char *cmdrsp_buf, u32 *cmdrsp_size)
{
    struct tls_hostif_cmdrsp *cmdrsp = (struct tls_hostif_cmdrsp *)cmdrsp_buf;
    struct tls_cmd_connect_t conn;
    enum tls_cmd_mode cmd_mode;
    int offset;
    u8 ext = 0;
    u8 err = 0;
	struct tls_hostif *hif = tls_get_hostif();

    if (hif->hostif_mode == HOSTIF_MODE_HSPI)
        cmd_mode = CMD_MODE_HSPI_RICMD;
    else
        cmd_mode = CMD_MODE_UART1_RICMD;        

    do {
        if (length != sizeof(struct tls_hostif_cmd_hdr)) {
            err = CMD_ERR_INV_PARAMS; 
            *cmdrsp_size = sizeof(struct tls_hostif_cmd_hdr);
            break;
        }

        memset(&conn, 0, sizeof(struct tls_cmd_connect_t));
        conn.res = 0;
        err = tls_cmd_join(cmd_mode, &conn);
        if (conn.res == 1) {
            /* fill connect network information */
            MEMCPY(cmdrsp->params.join.bssid, conn.bssid, ETH_ALEN);
            cmdrsp->params.join.type = conn.type;
            cmdrsp->params.join.channel = conn.channel;
            cmdrsp->params.join.encrypt = conn.encrypt;
            cmdrsp->params.join.ssid_len = conn.ssid_len;
            MEMCPY(cmdrsp->params.join.ssid, conn.ssid, conn.ssid_len);
            offset = sizeof(HOSTIF_CMDRSP_PARAMS_JOIN) + conn.ssid_len + 
                + sizeof(struct tls_hostif_cmd_hdr) + 
                sizeof(struct tls_hostif_hdr);
            *(cmdrsp_buf + offset) = conn.rssi; 
            *cmdrsp_size = offset + 1 - sizeof(struct tls_hostif_hdr);
            err = CMD_ERR_OK;
            ext = 1;
        } else {
            *cmdrsp_size = sizeof(struct tls_hostif_cmd_hdr);
            ext = 0; 
        }

    } while(0); 

    tls_hostif_fill_cmdrsp_hdr(cmdrsp, HOSTIF_CMD_WJOIN, err, ext);

    return 0; 
}

int ricmd_disconnect_proc(char *buf, u32 length, 
        char *cmdrsp_buf, u32 *cmdrsp_size)
{
    struct tls_hostif_cmdrsp *cmdrsp = (struct tls_hostif_cmdrsp *)cmdrsp_buf;
    u8 err = 0;

    do {
        if (length != sizeof(struct tls_hostif_cmd_hdr)) {
            err = CMD_ERR_INV_PARAMS; 
            break;
        }

        tls_cmd_disconnect_network();
    } while(0);

    *cmdrsp_size = sizeof(struct tls_hostif_cmd_hdr);
    tls_hostif_fill_cmdrsp_hdr(cmdrsp, HOSTIF_CMD_WLEAVE, err, 0); 

    return 0; 
}

int ricmd_scan_proc(char *buf, u32 length, 
        char *cmdrsp_buf, u32 *cmdrsp_size)
{
    struct tls_hostif_cmdrsp *cmdrsp = (struct tls_hostif_cmdrsp *)cmdrsp_buf;
    u8 err = CMD_ERR_OK;
    enum tls_cmd_mode cmd_mode;  
	struct tls_hostif *hif = tls_get_hostif();

    do {
        if (length != sizeof(struct tls_hostif_cmd_hdr)) {
            err = CMD_ERR_INV_PARAMS; 
            break;
        }
        
        if (hif->hostif_mode == HOSTIF_MODE_HSPI)
            cmd_mode = CMD_MODE_HSPI_RICMD;
        else
            cmd_mode = CMD_MODE_UART1_RICMD; 
        
        err = tls_cmd_scan(cmd_mode); 
    } while(0);

    *cmdrsp_size = sizeof(struct tls_hostif_cmd_hdr);
    tls_hostif_fill_cmdrsp_hdr(cmdrsp, HOSTIF_CMD_WSCAN, err, 0); 

    return 0; 
}

int ricmd_link_status_proc(char *buf, u32 length, 
        char *cmdrsp_buf, u32 *cmdrsp_size)
{
    struct tls_hostif_cmdrsp *cmdrsp = (struct tls_hostif_cmdrsp *)cmdrsp_buf;
    struct tls_cmd_link_status_t lks;
    u8 err = CMD_ERR_OK;
    u8 ext = 0;

    do {
        if (length != sizeof(struct tls_hostif_cmd_hdr)) {
            err = CMD_ERR_INV_PARAMS; 
            *cmdrsp_size = sizeof(struct tls_hostif_cmd_hdr);
            break;
        }

        tls_cmd_get_link_status(&lks); 

        cmdrsp->params.lkstt.status = lks.status;
        MEMCPY(cmdrsp->params.lkstt.ip, lks.ip, 4);
        MEMCPY(cmdrsp->params.lkstt.nm, lks.netmask, 4);
        MEMCPY(cmdrsp->params.lkstt.gw, lks.gw, 4);
        MEMCPY(cmdrsp->params.lkstt.dns1, lks.dns1, 4);
        MEMCPY(cmdrsp->params.lkstt.dns2, lks.dns2, 4);
        if (lks.status == 1)
            *cmdrsp_size = sizeof(struct tls_hostif_cmd_hdr) +
                sizeof(struct _HOSTIF_CMDRSP_PARAMS_LKSTT); 
        else 
            *cmdrsp_size = sizeof(struct tls_hostif_cmd_hdr) + 1;
        ext = 1; 
    } while(0);

    tls_hostif_fill_cmdrsp_hdr(cmdrsp, HOSTIF_CMD_LINK_STATUS, err, ext);

    return 0; 
}

int ricmd_wpsst_proc(char *buf, u32 length, 
        char *cmdrsp_buf, u32 *cmdrsp_size)
{
    struct tls_hostif_cmdrsp *cmdrsp = (struct tls_hostif_cmdrsp *)cmdrsp_buf;
    int err = CMD_ERR_OK;

    do {
        if (length != sizeof(struct tls_hostif_cmd_hdr)) {
            err = CMD_ERR_INV_PARAMS; 
            break;
        }

        err = tls_cmd_wps_start();
        if (err)
            err = CMD_ERR_INV_PARAMS; 
        
    } while(0);

    *cmdrsp_size = sizeof(struct tls_hostif_cmd_hdr);
    tls_hostif_fill_cmdrsp_hdr(cmdrsp, HOSTIF_CMD_WPSST, err, 0);

    return 0; 
}

int ricmd_skct_proc(char *buf, u32 length, 
        char *cmdrsp_buf, u32 *cmdrsp_size)
{
    struct tls_hostif_cmdrsp *cmdrsp = (struct tls_hostif_cmdrsp *)cmdrsp_buf;
    struct tls_hostif_cmd *cmd = (struct tls_hostif_cmd *)buf;
    int skt_num; 
    enum tls_cmd_mode cmd_mode;
    struct tls_cmd_socket_t skt;
    int err = CMD_ERR_INV_PARAMS;
    u8 ext;
    u8 *p;
	struct tls_hostif *hif = tls_get_hostif();

    do {
        length -= sizeof(struct tls_hostif_cmd_hdr);
        if (length > 1) {
            skt.proto = cmd->params.skct.proto;
            if (skt.proto > 1) 
                break;
        } else
            break;
        length -= 1;

        if (length > 1) {
            if (cmd->params.skct.server  > 1)
                break;
            skt.client = cmd->params.skct.server ? 0 : 1;
        } else
            break;
        length -= 1;

        if (length > 1) {
            if (cmd->params.skct.host_len > 31)
                break;
        } else
            break;

        if (length > 1)
            skt.host_len = cmd->params.skct.host_len;
        else
            break;
        length -= 1;

        if (length > skt.host_len) {
            p = &cmd->params.skct.host_len + 1;
            /* check ip or timeout  */
            if ((skt.client == 1) && (skt.host_len == 4)) {
                *(u32 *)skt.ip_addr = get_unaligned_le32(p); 
            } else if (skt.client == 1) {
                MEMCPY(skt.host_name, p, skt.host_len);
            } else if ((skt.client == 0) && (skt.proto == 0)) {
                /* TCP SERVER  */
                skt.timeout = get_unaligned_be32(p); 
            } else 
                ;
        } else
            break;
        length -= skt.host_len;

        if (length >= 2) {
            p = &cmd->params.skct.host_len + 1 + cmd->params.skct.host_len;
            skt.port = get_unaligned_be16(p);
        } else
            break;
        err = CMD_ERR_OK;
    } while (0);

    if (err) {
        err = CMD_ERR_INV_PARAMS;
        ext = 0;
        *cmdrsp_size = 0; 
    } else {
        if (hif->hostif_mode == HOSTIF_MODE_HSPI)
            cmd_mode = CMD_MODE_HSPI_RICMD;
        else
            cmd_mode = CMD_MODE_UART1_RICMD; 
        skt_num = tls_cmd_create_socket(&skt, cmd_mode);
        if (skt_num < 0 || skt_num > 20) {
            err = CMD_ERR_INV_PARAMS;
            ext = 0;
            *cmdrsp_size = 0;
        } else if (skt_num == 0) {
            err = CMD_ERR_NO_SKT;
            ext = 0;
            *cmdrsp_size = 0;
        } else {
            err = 0;
            ext = 1;
            cmdrsp->params.skct.socket = (u8 )skt_num;
            *cmdrsp_size = 1;
        } 
    } 

    tls_hostif_fill_cmdrsp_hdr(cmdrsp, HOSTIF_CMD_SKCT, err, ext);
    *cmdrsp_size += sizeof(struct tls_hostif_cmd_hdr);

    return 0; 
}

int ricmd_skstt_proc(char *buf, u32 length, 
        char *cmdrsp_buf, u32 *cmdrsp_size)
{
    struct tls_hostif_cmdrsp *cmdrsp = (struct tls_hostif_cmdrsp *)cmdrsp_buf;
    struct tls_hostif_cmd *cmd;
    struct tls_skt_status_t *skt_status;
    struct tls_skt_status_ext_t *cmd_status_ext;
    struct hostif_cmdrsp_skstt_ext *ext;
    u8 skt_num = 0;
    int i;
    int err = CMD_ERR_OK;
    u8 *skts_buf = NULL;
    u32 bufsize;
    int ret;

    do {
        if (length != sizeof(struct tls_hostif_cmd_hdr) + 1) {
            err = CMD_ERR_INV_PARAMS; 
            break;
        }

        cmd = (struct tls_hostif_cmd *)buf;

        if (cmd->cmd_hdr.ext == 0x1) {
            skt_num = cmd->params.skclose.socket;
            if (skt_num < 1 || skt_num > 4) {
                err = CMD_ERR_INV_PARAMS;
                break;
            }
        } else {
            err = CMD_ERR_INV_PARAMS;
            break;
        }


        bufsize = sizeof(struct tls_skt_status_ext_t) * 5 +
            sizeof(u32);
        skts_buf = tls_mem_alloc(bufsize);
        if (!skts_buf)
            err = CMD_ERR_MEM;
        else {
            ret = tls_cmd_get_socket_status(skt_num, skts_buf, bufsize); 
            if (ret)
                err = CMD_ERR_INV_PARAMS;
            skt_status = (struct tls_skt_status_t *)skts_buf;
            if (skt_status->socket_cnt == 0)
                err = CMD_ERR_OPS;
        }

    } while(0);

    if (err) {
        tls_hostif_fill_cmdrsp_hdr(cmdrsp, HOSTIF_CMD_SKSTT, err, 0);
        *cmdrsp_size = sizeof(struct tls_hostif_cmd_hdr);
    } else {
        tls_hostif_fill_cmdrsp_hdr(cmdrsp, HOSTIF_CMD_SKSTT, 0, 1);
        cmdrsp->params.skstt.number = skt_status->socket_cnt;
        ext = &cmdrsp->params.skstt.ext[0];
        cmd_status_ext = skt_status->skts_ext;
        *cmdrsp_size = sizeof(struct tls_hostif_cmd_hdr) + sizeof(u8); 
        for (i = 0; i<skt_status->socket_cnt; i++) {
            ext->socket = cmd_status_ext->socket;
            ext->status = cmd_status_ext->status;
            put_unaligned_le32(get_unaligned_le32(&cmd_status_ext->host_ipaddr[0]), 
                    (u8 *)&ext->host_ipaddr);
            put_unaligned_be16(get_unaligned_le16((u8 *)&cmd_status_ext->remote_port), 
                    (u8 *)&ext->remote_port);
            put_unaligned_be16(get_unaligned_le16((u8 *)&cmd_status_ext->local_port), 
                    (u8 *)&ext->local_port);
            *cmdrsp_size += sizeof(struct hostif_cmdrsp_skstt_ext); 
            cmd_status_ext++;
            ext++;
        }
    }
    if (skts_buf)
        tls_mem_free(skts_buf);

    return 0; 
}

int ricmd_skclose_proc(char *buf, u32 length, 
        char *cmdrsp_buf, u32 *cmdrsp_size)
{
    struct tls_hostif_cmdrsp *cmdrsp = (struct tls_hostif_cmdrsp *)cmdrsp_buf;
    struct tls_hostif_cmd *cmd;
    u8 skt_num = 0;
    u8 err = CMD_ERR_OK;
    int ret;

    cmd = (struct tls_hostif_cmd *)buf;

    do {
        if (length < sizeof(struct tls_hostif_cmd_hdr) + 1) {
            err = CMD_ERR_INV_PARAMS; 
            break;
        }

        if (cmd->cmd_hdr.ext == 0x1) {
            skt_num = cmd->params.skclose.socket;
            if (skt_num < 1 || skt_num > 20)
                err = CMD_ERR_INV_PARAMS;
        } else {
            err = CMD_ERR_INV_PARAMS;
        }

        if (!err) {
            ret = tls_cmd_close_socket(skt_num); 
            if (ret)
                err = CMD_ERR_OPS;
        }

    } while(0);


    tls_hostif_fill_cmdrsp_hdr(cmdrsp, HOSTIF_CMD_SKCLOSE, err, 0);
    *cmdrsp_size = sizeof(struct tls_hostif_cmd_hdr); 

    return 0; 
}

int ricmd_sksdf_proc(char *buf, u32 length, 
        char *cmdrsp_buf, u32 *cmdrsp_size)
{
    struct tls_hostif_cmdrsp *cmdrsp = (struct tls_hostif_cmdrsp *)cmdrsp_buf;
    struct tls_hostif_cmd *cmd;
    u8 skt_num = 0;
    int ret; 
    u8 err = 0;

    do {
        if (length != sizeof(struct tls_hostif_cmd_hdr) + 1) {
            err = CMD_ERR_INV_PARAMS; 
            break;
        }
        cmd = (struct tls_hostif_cmd *)buf;

        if (cmd->cmd_hdr.ext == 0x1) {
            skt_num = cmd->params.skclose.socket;
            if (skt_num < 1 || skt_num > 20)
                err = CMD_ERR_INV_PARAMS;
        } else {
            err = CMD_ERR_INV_PARAMS;
        } 

        if (!err) {
            ret = tls_cmd_set_default_socket(skt_num); 
            if (ret)
                err = CMD_ERR_INV_PARAMS;
        }
    } while(0);

    tls_hostif_fill_cmdrsp_hdr(cmdrsp, HOSTIF_CMD_SKSDF, err, 0);
    *cmdrsp_size = sizeof(struct tls_hostif_cmd_hdr); 

    return 0; 
}

int ricmd_wprt_proc(char *buf, u32 length, 
        char *cmdrsp_buf, u32 *cmdrsp_size)
{
    struct tls_hostif_cmdrsp *cmdrsp = (struct tls_hostif_cmdrsp *)cmdrsp_buf;
    struct tls_hostif_cmd *cmd;
    u8 err = CMD_ERR_OK;
    u8 update_flash = 0;
    int ret;
    u8 mode;

    cmd = (struct tls_hostif_cmd *)buf;
    do {
        if (cmd->cmd_hdr.ext & 0x1) {
            if (length != sizeof(struct tls_hostif_cmd_hdr) + 1) {
                err = CMD_ERR_INV_PARAMS; 
                break;
            }
            /*  set wireless network mode */
            if (cmd->cmd_hdr.ext & 0x2)
                update_flash = 1;
            mode = cmd->params.wprt.type;
            if (mode > 3) {
                err = CMD_ERR_INV_PARAMS;
                break;
            }

            ret = tls_cmd_set_wireless_mode(mode, update_flash);
            if (ret)
                err = CMD_ERR_OPS;
            tls_hostif_fill_cmdrsp_hdr(cmdrsp, HOSTIF_CMD_WPRT, err, 0);
            *cmdrsp_size = sizeof(struct tls_hostif_cmd_hdr);
        } else {
            if (length != sizeof(struct tls_hostif_cmd_hdr)) {
                err = CMD_ERR_INV_PARAMS; 
                break;
            }
            /*  get wireless network mode */
            tls_cmd_get_wireless_mode(&mode);
            cmdrsp->params.wprt.type = mode;
            tls_hostif_fill_cmdrsp_hdr(cmdrsp, HOSTIF_CMD_WPRT, 0, 1);
            *cmdrsp_size = sizeof(struct tls_hostif_cmd_hdr) + 1;
        } 
    } while (0);

    if (err) { 
        tls_hostif_fill_cmdrsp_hdr(cmdrsp, HOSTIF_CMD_WPRT, err, 0);
        *cmdrsp_size = sizeof(struct tls_hostif_cmd_hdr);
    }

    return 0; 
}

int ricmd_ssid_proc(char *buf, u32 length, 
        char *cmdrsp_buf, u32 *cmdrsp_size)
{
    struct tls_hostif_cmdrsp *cmdrsp = (struct tls_hostif_cmdrsp *)cmdrsp_buf;
    struct tls_hostif_cmd *cmd;
    u8 err = 0;
    u8 update_flash = 0;
    struct tls_cmd_ssid_t cmd_ssid;

    cmd = (struct tls_hostif_cmd *)buf;

    if (cmd->cmd_hdr.ext & 0x1) {
        /*  set ssid */
        if (cmd->cmd_hdr.ext & 0x2)
            update_flash = 1;

        if (cmd->params.ssid.ssid_len > 32 || cmd->params.ssid.ssid_len == 0)
            err = CMD_ERR_INV_PARAMS;
        else {
            cmd_ssid.ssid_len = cmd->params.ssid.ssid_len;
            MEMCPY(cmd_ssid.ssid, cmd->params.ssid.ssid, cmd_ssid.ssid_len);
            err = tls_cmd_set_ssid(&cmd_ssid, update_flash);
            if (err)
                err = CMD_ERR_INV_PARAMS;
        }

        tls_hostif_fill_cmdrsp_hdr(cmdrsp, HOSTIF_CMD_SSID, err, 0);
        *cmdrsp_size = sizeof(struct tls_hostif_cmd_hdr);
    } else {
        /*  get ssid */
        if (length != sizeof(struct tls_hostif_cmd_hdr)) {
            err = CMD_ERR_INV_PARAMS; 
        }
        if (!err) {
            tls_cmd_get_ssid(&cmd_ssid);
            cmdrsp->params.ssid.ssid_len = cmd_ssid.ssid_len;
            MEMCPY(cmdrsp->params.ssid.ssid, cmd_ssid.ssid, cmd_ssid.ssid_len);
            tls_hostif_fill_cmdrsp_hdr(cmdrsp, HOSTIF_CMD_SSID, 0, 1);
            *cmdrsp_size = sizeof(struct tls_hostif_cmd_hdr) + 1 + cmd_ssid.ssid_len;
        } else {
            tls_hostif_fill_cmdrsp_hdr(cmdrsp, HOSTIF_CMD_SSID, err, 0);
            *cmdrsp_size = sizeof(struct tls_hostif_cmd_hdr); 
        } 
    } 

    return 0; 
}

int ricmd_key_proc(char *buf, u32 length, 
        char *cmdrsp_buf, u32 *cmdrsp_size)
{
    struct tls_hostif_cmdrsp *cmdrsp = (struct tls_hostif_cmdrsp *)cmdrsp_buf;
    struct tls_hostif_cmd *cmd;
    u8 err = 0;
    u8 update_flash = 0;
    struct tls_cmd_key_t key;

    cmd = (struct tls_hostif_cmd *)buf;

    if (cmd->cmd_hdr.ext & 0x1) {
        /*  set key */
        if (cmd->cmd_hdr.ext & 0x2)
            update_flash = 1;
        key.format = cmd->params.key.format;
        key.index = cmd->params.key.index;
        key.key_len = cmd->params.key.key_len;
        if (key.key_len <= 64)
            MEMCPY(key.key, cmd->params.key.key, key.key_len);
        else {
            err = CMD_ERR_INV_PARAMS;
            tls_hostif_fill_cmdrsp_hdr(cmdrsp, HOSTIF_CMD_KEY, err, 0);
            *cmdrsp_size = sizeof(struct tls_hostif_cmd_hdr);
            return 0;
        }
        err = tls_cmd_set_key(&key, update_flash);
        if (err)
            err = CMD_ERR_INV_PARAMS;
        tls_hostif_fill_cmdrsp_hdr(cmdrsp, HOSTIF_CMD_KEY, err, 0);
        *cmdrsp_size = sizeof(struct tls_hostif_cmd_hdr);
    } else {
        /*  get key */
        if (length != sizeof(struct tls_hostif_cmd_hdr)) {
            err = CMD_ERR_INV_PARAMS; 
        }
        TLS_DBGPRT_INFO("err = %d\n", err);
        if (!err) {
            tls_cmd_get_key(&key);
            cmdrsp->params.key.format = key.format;
            *cmdrsp_size = 1;
            cmdrsp->params.key.index = key.index;
            *cmdrsp_size += 1;
            cmdrsp->params.key.key_len = key.key_len;
            *cmdrsp_size += 1;
            MEMCPY(cmdrsp->params.key.key, key.key, key.key_len);
            *cmdrsp_size += key.key_len;
            TLS_DBGPRT_INFO("cmdrsp size=%d, key_len = %d\n",
                    *cmdrsp_size, key.key_len);

            tls_hostif_fill_cmdrsp_hdr(cmdrsp, HOSTIF_CMD_KEY, 0, 1);
            *cmdrsp_size += sizeof(struct tls_hostif_cmd_hdr);
        } else {
            tls_hostif_fill_cmdrsp_hdr(cmdrsp, HOSTIF_CMD_KEY, err, 0);
            *cmdrsp_size = sizeof(struct tls_hostif_cmd_hdr);
        }
    } 
    return 0; 
}

int ricmd_encrypt_proc(char *buf, u32 length, 
        char *cmdrsp_buf, u32 *cmdrsp_size)
{
    struct tls_hostif_cmdrsp *cmdrsp = (struct tls_hostif_cmdrsp *)cmdrsp_buf;
    struct tls_hostif_cmd *cmd;
    u8 encrypt;
    u8 set_opt = 0;
    u8 err = CMD_ERR_OK;
    u8 update_flash = 0;

    cmd = (struct tls_hostif_cmd *)buf;

    if (cmd->cmd_hdr.ext & 0x1) {
        if (length == sizeof(struct tls_hostif_cmd_hdr) + 1) {
            encrypt = cmd->params.encrypt.mode;
            if (cmd->cmd_hdr.ext & 0x2)
                update_flash = 1;
            set_opt = 1;
        }  else 
            err = CMD_ERR_INV_PARAMS; 
    } else {
        if (length == sizeof(struct tls_hostif_cmd_hdr)) {
            set_opt = 0;
        } else  
            err = CMD_ERR_INV_PARAMS; 
    }

    if (!err) { 
        if (set_opt) {
            err = tls_cmd_set_encrypt(encrypt, update_flash); 
            if (err)
                err = CMD_ERR_INV_PARAMS;
            tls_hostif_fill_cmdrsp_hdr(cmdrsp, HOSTIF_CMD_ENCRYPT, err, 0);
            *cmdrsp_size = sizeof(struct tls_hostif_cmd_hdr);
        } else {
            tls_cmd_get_encrypt(&encrypt);
            cmdrsp->params.encrypt.mode = encrypt;
            tls_hostif_fill_cmdrsp_hdr(cmdrsp, HOSTIF_CMD_ENCRYPT, 0, 1);
            *cmdrsp_size = sizeof(struct tls_hostif_cmd_hdr) + 1;
        }
    }  else {
        tls_hostif_fill_cmdrsp_hdr(cmdrsp, HOSTIF_CMD_ENCRYPT, err, 0);
        *cmdrsp_size = sizeof(struct tls_hostif_cmd_hdr); 
    }

    return 0; 
}

int ricmd_bssid_proc(char *buf, u32 length, 
        char *cmdrsp_buf, u32 *cmdrsp_size)
{
    struct tls_hostif_cmdrsp *cmdrsp = (struct tls_hostif_cmdrsp *)cmdrsp_buf;
    struct tls_hostif_cmd *cmd;
    struct tls_cmd_bssid_t bssid;
    u8 set_opt = 0;
    u8 err = CMD_ERR_OK;
    u8 update_flash = 0;

    cmd = (struct tls_hostif_cmd *)buf;

    if (cmd->cmd_hdr.ext & 0x1) {
        if (length == sizeof(struct tls_hostif_cmd_hdr) + 1) {
            MEMCPY(bssid.bssid, cmd->params.bssid.bssid, ETH_ALEN);
            bssid.enable = cmd->params.bssid.enable;
            if (cmd->cmd_hdr.ext & 0x2)
                update_flash = 1;
            set_opt = 1;
        }  else 
            err = CMD_ERR_INV_PARAMS; 
    } else {
        if (length == sizeof(struct tls_hostif_cmd_hdr)) {
            set_opt = 0;
        } else  
            err = CMD_ERR_INV_PARAMS; 
    }

    if (!err) { 
        if (set_opt) {
            err = tls_cmd_set_bssid(&bssid, update_flash); 
            if (err)
                err = CMD_ERR_INV_PARAMS;
            tls_hostif_fill_cmdrsp_hdr(cmdrsp, HOSTIF_CMD_BSSID, err, 0);
            *cmdrsp_size = sizeof(struct tls_hostif_cmd_hdr);
        } else {
            tls_cmd_get_bssid(&bssid);
            MEMCPY(cmdrsp->params.bssid.bssid, bssid.bssid, ETH_ALEN);
            cmdrsp->params.bssid.enable = bssid.enable;
            tls_hostif_fill_cmdrsp_hdr(cmdrsp, HOSTIF_CMD_BSSID, 0, 1);
            *cmdrsp_size = sizeof(struct tls_hostif_cmd_hdr) + 7;
        }
    }  else {
        tls_hostif_fill_cmdrsp_hdr(cmdrsp, HOSTIF_CMD_BSSID, err, 0);
        *cmdrsp_size = sizeof(struct tls_hostif_cmd_hdr); 
    } 

    return 0; 
}

int ricmd_broad_ssid_proc(char *buf, u32 length, 
        char *cmdrsp_buf, u32 *cmdrsp_size)
{
    struct tls_hostif_cmdrsp *cmdrsp = (struct tls_hostif_cmdrsp *)cmdrsp_buf;
    struct tls_hostif_cmd *cmd;
    u8 set_opt = 0;
    u8 err = CMD_ERR_OK;
    u8 update_flash = 0;
    u8 ssid_set = 0;

    cmd = (struct tls_hostif_cmd *)buf;

    if (cmd->cmd_hdr.ext & 0x1) {
        if (length == sizeof(struct tls_hostif_cmd_hdr) + 1) {
            ssid_set = cmd->params.brd_ssid.enable ? 1 : 0;
            if (cmd->cmd_hdr.ext & 0x2)
                update_flash = 1;
            set_opt = 1;
        }  else 
            err = CMD_ERR_INV_PARAMS; 
    } else {
        if (length == sizeof(struct tls_hostif_cmd_hdr)) {
            set_opt = 0;
        } else  
            err = CMD_ERR_INV_PARAMS; 
    }

    if (!err) { 
        if (set_opt) {
            err = tls_cmd_set_hide_ssid(ssid_set, update_flash); 
            if (err)
                err = CMD_ERR_INV_PARAMS;
            tls_hostif_fill_cmdrsp_hdr(cmdrsp, HOSTIF_CMD_BRD_SSID, err, 0);
            *cmdrsp_size = sizeof(struct tls_hostif_cmd_hdr);
        } else {
            tls_cmd_get_hide_ssid(&ssid_set);
            cmdrsp->params.brd_ssid.enable = ssid_set ? 1 : 0;
            tls_hostif_fill_cmdrsp_hdr(cmdrsp, HOSTIF_CMD_BRD_SSID, 0, 1);
            *cmdrsp_size = sizeof(struct tls_hostif_cmd_hdr) + 1;
        }
    }  else {
        tls_hostif_fill_cmdrsp_hdr(cmdrsp, HOSTIF_CMD_BRD_SSID, err, 0);
        *cmdrsp_size = sizeof(struct tls_hostif_cmd_hdr); 
    } 

    return 0; 
}

int ricmd_chnl_proc(char *buf, u32 length, 
        char *cmdrsp_buf, u32 *cmdrsp_size)
{
    struct tls_hostif_cmdrsp *cmdrsp = (struct tls_hostif_cmdrsp *)cmdrsp_buf;
    struct tls_hostif_cmd *cmd;
    u8 set_opt = 0;
    u8 err = CMD_ERR_OK;
    u8 update_flash = 0;
    u8 channel;
    u8 channel_en;

    cmd = (struct tls_hostif_cmd *)buf;

    if (cmd->cmd_hdr.ext & 0x1) {
        if (length == sizeof(struct tls_hostif_cmd_hdr) + 2) {
            channel = cmd->params.channel.channel;
            channel_en = cmd->params.channel.enable;
            if (cmd->cmd_hdr.ext & 0x2)
                update_flash = 1;
            set_opt = 1;
            if (channel < 1 || channel > 14 || channel_en > 1)
                err = CMD_ERR_INV_PARAMS;
        }  else 
            err = CMD_ERR_INV_PARAMS; 
    } else {
        if (length == sizeof(struct tls_hostif_cmd_hdr)) {
            set_opt = 0;
        } else  
            err = CMD_ERR_INV_PARAMS; 
    }

    if (!err) { 
        if (set_opt) {
            err = tls_cmd_set_channel(channel, channel_en, update_flash); 
            if (err)
                err = CMD_ERR_INV_PARAMS;
            tls_hostif_fill_cmdrsp_hdr(cmdrsp, HOSTIF_CMD_CHNL, err, 0);
            *cmdrsp_size = sizeof(struct tls_hostif_cmd_hdr);
        } else {
            tls_cmd_get_channel(&channel, &channel_en);
            cmdrsp->params.channel.enable = channel_en;
            cmdrsp->params.channel.channel = channel;
            tls_hostif_fill_cmdrsp_hdr(cmdrsp, HOSTIF_CMD_CHNL, 0, 1);
            *cmdrsp_size = sizeof(struct tls_hostif_cmd_hdr) + 2;
        }
    }  else {
        tls_hostif_fill_cmdrsp_hdr(cmdrsp, HOSTIF_CMD_CHNL, err, 0);
        *cmdrsp_size = sizeof(struct tls_hostif_cmd_hdr); 
    } 

    return 0; 
}

int ricmd_wreg_proc(char *buf, u32 length, 
        char *cmdrsp_buf, u32 *cmdrsp_size)
{
    struct tls_hostif_cmdrsp *cmdrsp = (struct tls_hostif_cmdrsp *)cmdrsp_buf;
    struct tls_hostif_cmd *cmd;
    u8 set_opt = 0;
    u8 err = CMD_ERR_OK;
    u8 update_flash = 0;
    u16 region;

    cmd = (struct tls_hostif_cmd *)buf;

    if (cmd->cmd_hdr.ext & 0x1) {
        if (length == sizeof(struct tls_hostif_cmd_hdr) + 2) {
            region = be_to_host16(cmd->params.wreg.region);
            if (cmd->cmd_hdr.ext & 0x2)
                update_flash = 1;
            set_opt = 1;
        }  else 
            err = CMD_ERR_INV_PARAMS; 
    } else {
        if (length == sizeof(struct tls_hostif_cmd_hdr)) {
            set_opt = 0;
        } else  
            err = CMD_ERR_INV_PARAMS; 
    }

    if (!err) { 
        if (set_opt) {
            err = tls_cmd_set_region(region, update_flash); 
            if (err)
                err = CMD_ERR_INV_PARAMS;
            tls_hostif_fill_cmdrsp_hdr(cmdrsp, HOSTIF_CMD_WREG, err, 0);
            *cmdrsp_size = sizeof(struct tls_hostif_cmd_hdr);
        } else {
            tls_cmd_get_region(&region);
            cmdrsp->params.wreg.region = host_to_be16(region);
            tls_hostif_fill_cmdrsp_hdr(cmdrsp, HOSTIF_CMD_WREG, 0, 1);
            *cmdrsp_size = sizeof(struct tls_hostif_cmd_hdr) + 1;
        }
    }  else {
        tls_hostif_fill_cmdrsp_hdr(cmdrsp, HOSTIF_CMD_WREG, err, 0);
        *cmdrsp_size = sizeof(struct tls_hostif_cmd_hdr); 
    } 

    return 0; 
}

int ricmd_wbgr_proc(char *buf, u32 length, 
        char *cmdrsp_buf, u32 *cmdrsp_size)
{
    struct tls_hostif_cmdrsp *cmdrsp = (struct tls_hostif_cmdrsp *)cmdrsp_buf;
    struct tls_hostif_cmd *cmd;
    u8 set_opt = 0;
    u8 err = CMD_ERR_OK;
    int ret;
    u8 update_flash = 0;
    struct tls_cmd_wl_hw_mode_t wl_hw_mode;

    cmd = (struct tls_hostif_cmd *)buf;

    if (cmd->cmd_hdr.ext & 0x1) {
        if (length == sizeof(struct tls_hostif_cmd_hdr) + 2) {
            wl_hw_mode.hw_mode = cmd->params.wbgr.mode;
            wl_hw_mode.max_rate = cmd->params.wbgr.rate;
            if (cmd->cmd_hdr.ext & 0x2)
                update_flash = 1;
            set_opt = 1;
        }  else 
            err = CMD_ERR_INV_PARAMS; 
    } else {
        if (length == sizeof(struct tls_hostif_cmd_hdr)) {
            set_opt = 0;
        } else  
            err = CMD_ERR_INV_PARAMS; 
    }

    if (!err) { 
        if (set_opt) {
            ret = tls_cmd_set_hw_mode(&wl_hw_mode, update_flash); 
            if (ret)
                err = CMD_ERR_INV_PARAMS;
            tls_hostif_fill_cmdrsp_hdr(cmdrsp, HOSTIF_CMD_WBGR, err, 0);
            *cmdrsp_size = sizeof(struct tls_hostif_cmd_hdr);
        } else {
            tls_cmd_get_hw_mode(&wl_hw_mode);
            cmdrsp->params.wbgr.mode = wl_hw_mode.hw_mode;
            cmdrsp->params.wbgr.rate = wl_hw_mode.max_rate;
            tls_hostif_fill_cmdrsp_hdr(cmdrsp, HOSTIF_CMD_WBGR, 0, 1);
            *cmdrsp_size = sizeof(struct tls_hostif_cmd_hdr) + 2;
        }
    }  else {
        tls_hostif_fill_cmdrsp_hdr(cmdrsp, HOSTIF_CMD_WBGR, err, 0);
        *cmdrsp_size = sizeof(struct tls_hostif_cmd_hdr); 
    } 

    return 0; 
}

int ricmd_watc_proc(char *buf, u32 length, 
        char *cmdrsp_buf, u32 *cmdrsp_size)
{
    struct tls_hostif_cmdrsp *cmdrsp = (struct tls_hostif_cmdrsp *)cmdrsp_buf;
    struct tls_hostif_cmd *cmd;
    u8 set_opt = 0;
    u8 err = CMD_ERR_OK;
    u8 update_flash = 0;
    u8 mode;

    cmd = (struct tls_hostif_cmd *)buf;

    if (cmd->cmd_hdr.ext & 0x1) {
        if (length == sizeof(struct tls_hostif_cmd_hdr) + 1) {
            mode = cmd->params.watc.enable;
            if (cmd->cmd_hdr.ext & 0x2)
                update_flash = 1;
            set_opt = 1;
        }  else 
            err = CMD_ERR_INV_PARAMS; 
    } else {
        if (length == sizeof(struct tls_hostif_cmd_hdr)) {
            set_opt = 0;
        } else  
            err = CMD_ERR_INV_PARAMS; 
    }

    if (!err) { 
        if (set_opt) {
            err = tls_cmd_set_adhoc_create_mode(mode, update_flash); 
            if (err)
                err = CMD_ERR_INV_PARAMS;
            tls_hostif_fill_cmdrsp_hdr(cmdrsp, HOSTIF_CMD_WATC, err, 0);
            *cmdrsp_size = sizeof(struct tls_hostif_cmd_hdr);
        } else {
            tls_cmd_get_adhoc_create_mode(&mode);
            cmdrsp->params.watc.enable = mode;
            tls_hostif_fill_cmdrsp_hdr(cmdrsp, HOSTIF_CMD_WATC, 0, 1);
            *cmdrsp_size = sizeof(struct tls_hostif_cmd_hdr) + 1;
        }
    }  else {
        tls_hostif_fill_cmdrsp_hdr(cmdrsp, HOSTIF_CMD_WATC, err, 0);
        *cmdrsp_size = sizeof(struct tls_hostif_cmd_hdr); 
    } 

    return 0; 
}

int ricmd_wpsm_proc(char *buf, u32 length, 
        char *cmdrsp_buf, u32 *cmdrsp_size)
{
    struct tls_hostif_cmdrsp *cmdrsp = (struct tls_hostif_cmdrsp *)cmdrsp_buf;
    struct tls_hostif_cmd *cmd;
    u8 set_opt = 0;
    u8 err = 0;
    u8 update_flash = 0;
    u8 ps_enable;

    cmd = (struct tls_hostif_cmd *)buf;

    if (cmd->cmd_hdr.ext & 0x1) {
        if (length == sizeof(struct tls_hostif_cmd_hdr) + 1) {
            ps_enable = cmd->params.wpsm.enable;
            if (cmd->cmd_hdr.ext & 0x2)
                update_flash = 1;
            set_opt = 1;
        }  else 
            err = CMD_ERR_INV_PARAMS; 
    } else {
        if (length == sizeof(struct tls_hostif_cmd_hdr)) {
            set_opt = 0;
        } else  
            err = CMD_ERR_INV_PARAMS; 
    }

    if (!err) { 
        if (set_opt) {
            err = tls_cmd_set_wl_ps_mode(ps_enable, update_flash); 
            if (err)
                err = CMD_ERR_INV_PARAMS;
            tls_hostif_fill_cmdrsp_hdr(cmdrsp, HOSTIF_CMD_WPSM, err, 0);
            *cmdrsp_size = sizeof(struct tls_hostif_cmd_hdr);
        } else {
            tls_cmd_get_wl_ps_mode(&ps_enable);
            cmdrsp->params.wpsm.enable = ps_enable;
            tls_hostif_fill_cmdrsp_hdr(cmdrsp, HOSTIF_CMD_WPSM, 0, 1);
            *cmdrsp_size = sizeof(struct tls_hostif_cmd_hdr) + 1;
        }
    }  else {
        tls_hostif_fill_cmdrsp_hdr(cmdrsp, HOSTIF_CMD_WPSM, err, 0);
        *cmdrsp_size = sizeof(struct tls_hostif_cmd_hdr); 
    } 

    return 0; 
}

int ricmd_warm_proc(char *buf, u32 length, 
        char *cmdrsp_buf, u32 *cmdrsp_size)
{
    struct tls_hostif_cmdrsp *cmdrsp = (struct tls_hostif_cmdrsp *)cmdrsp_buf;
    struct tls_hostif_cmd *cmd;
    u8 set_opt = 0;
    u8 err = 0;
    u8 update_flash = 0;
    u8 roaming_enable;

    cmd = (struct tls_hostif_cmd *)buf;

    if (cmd->cmd_hdr.ext & 0x1) {
        if (length == sizeof(struct tls_hostif_cmd_hdr) + 1) {
            roaming_enable = cmd->params.warm.enable;
            if (cmd->cmd_hdr.ext & 0x2)
                update_flash = 1;
            set_opt = 1;
        }  else 
            err = CMD_ERR_INV_PARAMS; 
    } else {
        if (length == sizeof(struct tls_hostif_cmd_hdr)) {
            set_opt = 0;
        } else  
            err = CMD_ERR_INV_PARAMS; 
    }

    if (!err) { 
        if (set_opt) {
            err = tls_cmd_set_roaming_mode(roaming_enable, update_flash); 
            if (err)
                err = CMD_ERR_INV_PARAMS;
            tls_hostif_fill_cmdrsp_hdr(cmdrsp, HOSTIF_CMD_WARM, err, 0);
            *cmdrsp_size = sizeof(struct tls_hostif_cmd_hdr);
        } else {
            tls_cmd_get_roaming_mode(&roaming_enable);
            cmdrsp->params.warm.enable = roaming_enable;
            tls_hostif_fill_cmdrsp_hdr(cmdrsp, HOSTIF_CMD_WARM, 0, 1);
            *cmdrsp_size = sizeof(struct tls_hostif_cmd_hdr) + 1;
        }
    }  else {
        tls_hostif_fill_cmdrsp_hdr(cmdrsp, HOSTIF_CMD_WARM, err, 0);
        *cmdrsp_size = sizeof(struct tls_hostif_cmd_hdr); 
    } 

    return 0; 
}

int ricmd_wps_proc(char *buf, u32 length, 
        char *cmdrsp_buf, u32 *cmdrsp_size)
{
    struct tls_hostif_cmdrsp *cmdrsp = (struct tls_hostif_cmdrsp *)cmdrsp_buf;
    struct tls_hostif_cmd *cmd;
    u8 set_opt = 0;
    u8 err = CMD_ERR_OK;
    u8 update_flash = 0;
    struct tls_cmd_wps_params_t params;

    cmd = (struct tls_hostif_cmd *)buf;

    if (cmd->cmd_hdr.ext & 0x1) {
        if (length == sizeof(struct tls_hostif_cmd_hdr) + 10) {
            params.mode = cmd->params.wps.mode;
            params.pin_len = cmd->params.wps.pin_len;
            MEMCPY(params.pin, cmd->params.wps.pin, 8);
            if (cmd->cmd_hdr.ext & 0x2)
                update_flash = 1;
            set_opt = 1;
        }  else 
            err = CMD_ERR_INV_PARAMS; 
    } else {
        if (length == sizeof(struct tls_hostif_cmd_hdr)) {
            set_opt = 0;
        } else  
            err = CMD_ERR_INV_PARAMS; 
    }

    if (!err) { 
        if (set_opt) {
            err = tls_cmd_set_wps_params(&params, update_flash); 
            if (err)
                err = CMD_ERR_INV_PARAMS;
            tls_hostif_fill_cmdrsp_hdr(cmdrsp, HOSTIF_CMD_WPS, err, 0);
            *cmdrsp_size = sizeof(struct tls_hostif_cmd_hdr);
        } else {
            tls_cmd_get_wps_params(&params);
            cmdrsp->params.wps.mode = params.mode;
            cmdrsp->params.wps.pin_len = params.pin_len;
            MEMCPY(cmdrsp->params.wps.pin, params.pin, params.pin_len);
            tls_hostif_fill_cmdrsp_hdr(cmdrsp, HOSTIF_CMD_WPS, 0, 1);
            *cmdrsp_size = sizeof(struct tls_hostif_cmd_hdr) + 2 + params.pin_len;
        }
    }  else {
        tls_hostif_fill_cmdrsp_hdr(cmdrsp, HOSTIF_CMD_WPS, err, 0);
        *cmdrsp_size = sizeof(struct tls_hostif_cmd_hdr); 
    } 

    return 0; 
}

int ricmd_nip_proc(char *buf, u32 length, 
        char *cmdrsp_buf, u32 *cmdrsp_size)
{
    struct tls_hostif_cmdrsp *cmdrsp = (struct tls_hostif_cmdrsp *)cmdrsp_buf;
    struct tls_hostif_cmd *cmd;
    u8 set_opt = 0;
    u8 err = CMD_ERR_OK;
    int ret;
    u8 update_flash = 0;
    struct tls_cmd_ip_params_t params;

    cmd = (struct tls_hostif_cmd *)buf;

    if (cmd->cmd_hdr.ext & 0x1) {
        if (length == sizeof(struct tls_hostif_cmd_hdr) + 17) {
            MEMCPY(params.ip_addr, cmd->params.nip.ip, 4);
            MEMCPY(params.netmask, cmd->params.nip.nm, 4);
            MEMCPY(params.gateway, cmd->params.nip.gw, 4);
            MEMCPY(params.dns, cmd->params.nip.dns, 4);
            params.type = cmd->params.nip.type;
            if (params.type > 1) {
                err = CMD_ERR_INV_PARAMS;
            } else {
                if (cmd->cmd_hdr.ext & 0x2)
                    update_flash = 1;
                set_opt = 1;
            }
        } else
            err = CMD_ERR_INV_PARAMS; 
    } else {
        if (length == sizeof(struct tls_hostif_cmd_hdr)) {
            set_opt = 0;
        } else  
            err = CMD_ERR_INV_PARAMS; 
    }

    if (!err) { 
        if (set_opt) {
            ret = tls_cmd_set_ip_info(&params, update_flash); 
            if (ret)
                err = CMD_ERR_INV_PARAMS;
            tls_hostif_fill_cmdrsp_hdr(cmdrsp, HOSTIF_CMD_NIP, err, 0);
            *cmdrsp_size = sizeof(struct tls_hostif_cmd_hdr);
        } else {
            tls_cmd_get_ip_info(&params);
            cmdrsp->params.nip.type = params.type;
            MEMCPY(cmdrsp->params.nip.gw, params.gateway, 4);
            MEMCPY(cmdrsp->params.nip.ip, params.ip_addr, 4);
            MEMCPY(cmdrsp->params.nip.nm, params.netmask, 4);
            MEMCPY(cmdrsp->params.nip.dns, params.dns, 4);
            tls_hostif_fill_cmdrsp_hdr(cmdrsp, HOSTIF_CMD_NIP, 0, 1);
            if (params.type == 0) {
                *cmdrsp_size = sizeof(struct tls_hostif_cmd_hdr) + 1;
            } else {
                *cmdrsp_size = sizeof(struct tls_hostif_cmd_hdr) + 17; 
            }
        }
    }  else {
        tls_hostif_fill_cmdrsp_hdr(cmdrsp, HOSTIF_CMD_NIP, err, 0);
        *cmdrsp_size = sizeof(struct tls_hostif_cmd_hdr); 
    } 

    return 0; 
}

int ricmd_atm_proc(char *buf, u32 length, 
        char *cmdrsp_buf, u32 *cmdrsp_size)
{
    struct tls_hostif_cmdrsp *cmdrsp = (struct tls_hostif_cmdrsp *)cmdrsp_buf;
    struct tls_hostif_cmd *cmd;
    u8 set_opt = 0;
    u8 err = CMD_ERR_OK;
    u8 update_flash = 0;
    u8 mode;

    cmd = (struct tls_hostif_cmd *)buf;

    if (cmd->cmd_hdr.ext & 0x1) {
        if (length == sizeof(struct tls_hostif_cmd_hdr) + 1) {
            mode = cmd->params.atm.mode;
            if (cmd->cmd_hdr.ext & 0x2)
                update_flash = 1;
            set_opt = 1;
        }  else 
            err = CMD_ERR_INV_PARAMS; 
    } else {
        if (length == sizeof(struct tls_hostif_cmd_hdr)) {
            set_opt = 0;
        } else  
            err = CMD_ERR_INV_PARAMS; 
    }

    if (!err) { 
        if (set_opt) {
            err = tls_cmd_set_work_mode(mode, update_flash); 
            if (err)
                err = CMD_ERR_INV_PARAMS;
            tls_hostif_fill_cmdrsp_hdr(cmdrsp, HOSTIF_CMD_ATM, err, 0);
            *cmdrsp_size = sizeof(struct tls_hostif_cmd_hdr);
        } else {
            tls_cmd_get_work_mode(&mode);
            cmdrsp->params.atm.mode = mode;
            tls_hostif_fill_cmdrsp_hdr(cmdrsp, HOSTIF_CMD_ATM, 0, 1);
            *cmdrsp_size = sizeof(struct tls_hostif_cmd_hdr) + 1;
        }
    }  else {
        tls_hostif_fill_cmdrsp_hdr(cmdrsp, HOSTIF_CMD_ATM, err, 0);
        *cmdrsp_size = sizeof(struct tls_hostif_cmd_hdr); 
    } 

    return 0; 
}

int ricmd_atrm_proc(char *buf, u32 length, 
        char *cmdrsp_buf, u32 *cmdrsp_size)
{
    struct tls_hostif_cmdrsp *cmdrsp = (struct tls_hostif_cmdrsp *)cmdrsp_buf;
    struct tls_hostif_cmd *cmd = (struct tls_hostif_cmd *)buf;
    int err = CMD_ERR_INV_PARAMS;
    struct tls_cmd_socket_t skt;
    u8 ext;
    u8 set_opt;
    u8 *p;
    u8 update_flash = 0;

    memset(&skt, 0, sizeof(struct tls_cmd_socket_t));

    if (cmd->cmd_hdr.ext & 0x1) {
        if (cmd->cmd_hdr.ext & 0x2)
            update_flash = 1;
        set_opt = 1;
    } else {
        if (length == sizeof(struct tls_hostif_cmd_hdr)) {
            set_opt = 0;
        } else  
            err = CMD_ERR_INV_PARAMS; 
    }

    if (!err) {
        if (set_opt) { 
            do {
                err = CMD_ERR_INV_PARAMS;
                length -= sizeof(struct tls_hostif_cmd_hdr);
                if (length > 1) {
                    skt.proto = cmd->params.atrm.proto;
                    if (skt.proto > 1) {
                        break;
                    }
                }
                length -= 1;

                if (length > 1) {
                    if (cmd->params.atrm.server > 1)
                        break;
                    skt.client = cmd->params.atrm.server ? 0 : 1;
                } else
                    break;
                length -= 1;

                if (length > 1) {
                    if (cmd->params.atrm.host_len > 31)
                        break;
                } else
                    break;
                length -= 1;

                if (length > 1)
                    skt.host_len = cmd->params.atrm.host_len;
                else
                    break;
                length -= 1;

                if (length > skt.host_len) {
                    p = &cmd->params.atrm.host_len + 1;
                    /* check ip or timeout  */
                    if ((skt.client == 1) && (skt.host_len == 4)) {
                        *(u32 *)skt.ip_addr = get_unaligned_be32(p); 
                    } else if (skt.client == 1) {
                        MEMCPY(skt.host_name, p, skt.host_len); 
                    } else if ((skt.client == 0) && (skt.proto == 0)){
                        /* TCP SERVER  */
                        skt.timeout = get_unaligned_be32(p); 
                    } else {
                        MEMCPY(skt.host_name, cmd->params.atrm.host,
                                cmd->params.atrm.host_len); 
                    }
                } else
                    break;
                length -= skt.host_len;

                if (length == 2) {
                    p = &cmd->params.skct.host_len + 1 + cmd->params.atrm.host_len;
                    skt.port = get_unaligned_be16(p);
                } else
                    break;
                err = CMD_ERR_OK;
            } while(0);
            if (!err) {
                tls_cmd_set_default_socket_params(&skt, update_flash);
                ext = 0; 
            } else {
                ext = 0;
                *cmdrsp_size = sizeof(struct tls_hostif_cmd_hdr); 
            }
        } else {
            /* get default socket info */
            tls_cmd_get_default_socket_params(&skt);
            cmdrsp->params.atrm.proto = skt.proto;
            cmdrsp->params.atrm.server = skt.client ? 0 : 1;
            cmdrsp->params.atrm.host_len = skt.host_len;
            p = &cmdrsp->params.atrm.host[0];
            if ((skt.host_len == 4) && (skt.client)) {
                MEMCPY(p, skt.ip_addr, skt.host_len);
            } else if (skt.client) {
                MEMCPY(p, skt.host_name, skt.host_len);
            } else if (!skt.client && skt.proto == 0) {
                put_unaligned_be32(skt.timeout, (u8 *)p);
            } else
                ;
            p += skt.host_len;
            put_unaligned_be16(skt.port, (u8 *)p);
            p += 2;
            ext = 1;
            *cmdrsp_size = sizeof(struct tls_hostif_cmd_hdr) + (p-&cmdrsp->params.atrm.proto); 
        }
    } else {
        ext = 0;
        *cmdrsp_size = sizeof(struct tls_hostif_cmd_hdr); 
    }

    tls_hostif_fill_cmdrsp_hdr(cmdrsp, HOSTIF_CMD_ATRM, err, ext);
    *cmdrsp_size += sizeof(struct tls_hostif_cmd_hdr);

    return 0; 
}

int ricmd_portm_proc(char *buf, u32 length, 
        char *cmdrsp_buf, u32 *cmdrsp_size)
{
    struct tls_hostif_cmdrsp *cmdrsp = (struct tls_hostif_cmdrsp *)cmdrsp_buf;
    struct tls_hostif_cmd *cmd;
    u8 port_mode;
    u8 set_opt = 0;
    u8 err = CMD_ERR_OK;
    u8 update_flash = 0;

    cmd = (struct tls_hostif_cmd *)buf;

    if (cmd->cmd_hdr.ext & 0x1) {
        if (length == sizeof(struct tls_hostif_cmd_hdr) + 1) {
            port_mode = cmd->params.portm.mode;
            if (cmd->cmd_hdr.ext & 0x2)
                update_flash = 1;
            set_opt = 1;
        }  else 
            err = CMD_ERR_INV_PARAMS; 

        if (port_mode > 2)
            err = CMD_ERR_INV_PARAMS;
    } else {
        if (length == sizeof(struct tls_hostif_cmd_hdr)) {
            set_opt = 0;
        } else  
            err = CMD_ERR_INV_PARAMS; 
    }

    if (!err) { 
        if (set_opt) {
            err = tls_cmd_set_hostif_mode(port_mode, update_flash); 
            if (err)
                err = CMD_ERR_INV_PARAMS;
            tls_hostif_fill_cmdrsp_hdr(cmdrsp, HOSTIF_CMD_PORTM, err, 0);
            *cmdrsp_size = sizeof(struct tls_hostif_cmd_hdr);
        } else {
            tls_cmd_get_hostif_mode(&port_mode);
            cmdrsp->params.portm.mode = port_mode;
            tls_hostif_fill_cmdrsp_hdr(cmdrsp, HOSTIF_CMD_PORTM, 0, 1);
            *cmdrsp_size = sizeof(struct tls_hostif_cmd_hdr) + 1;
        }
    }  else {
        tls_hostif_fill_cmdrsp_hdr(cmdrsp, HOSTIF_CMD_PORTM, err, 0);
        *cmdrsp_size = sizeof(struct tls_hostif_cmd_hdr); 
    }

    return 0; 
}

int ricmd_uart_proc(char *buf, u32 length, 
        char *cmdrsp_buf, u32 *cmdrsp_size)
{
    struct tls_hostif_cmdrsp *cmdrsp = (struct tls_hostif_cmdrsp *)cmdrsp_buf;
    struct tls_hostif_cmd *cmd = (struct tls_hostif_cmd *)buf;
    int err = CMD_ERR_OK;
    struct tls_cmd_uart_params_t params;
    u8 ext;
    u8 set_opt;
    u8 *p;
    u8 update_flash = 0;

    memset(&params, 0, sizeof(struct tls_cmd_uart_params_t));

    if (cmd->cmd_hdr.ext & 0x1) {
        if (cmd->cmd_hdr.ext & 0x2)
            update_flash = 1;
        set_opt = 1;
    } else {
        if (length == sizeof(struct tls_hostif_cmd_hdr)) {
            set_opt = 0;
        } else  
            err = CMD_ERR_INV_PARAMS; 
    }

    if (err == CMD_ERR_OK) {
        if (set_opt) { 
            do {
                err = CMD_ERR_INV_PARAMS;
                length -= sizeof(struct tls_hostif_cmd_hdr);
                p = (u8 *)(buf + sizeof(struct tls_hostif_cmd_hdr));

                /* baud rate */
                if (length > 3) {
                    params.baud_rate = be_to_host32(get_unaligned_le32(p) >> 4);
                } else
                    break;

                length -= 3;
                p += 3;
                /* char length */
                if (length > 1) {
                    params.charlength = *p++;
                } else
                    break;

                length -= 1;
                /* stop bit */
                if (length > 1) {
                    params.stop_bit = *p++;
                } else
                    break;

                length -= 1;
                /* parity */
                if (length > 1) {
                    params.parity = *p++;
                } else
                    break;

                length -= 1;
                /* flow control */ 
                if (length == 1) {
                    params.flow_ctrl = *p++;
                } else
                    break;

                err = CMD_ERR_OK;
            } while(0);

            if (!err) {
                tls_cmd_set_uart_params(&params, update_flash);
                ext = 0; 
            } else {
                ext = 0;
                *cmdrsp_size = sizeof(struct tls_hostif_cmd_hdr); 
            }
        } else {
            tls_cmd_get_uart_params(&params);
            p = (u8 *)&params.baud_rate;
            cmdrsp->params.uart.baud_rate[0] = p[2];
            cmdrsp->params.uart.baud_rate[1] = p[1];
            cmdrsp->params.uart.baud_rate[2] = p[0];
            cmdrsp->params.uart.char_len = params.charlength;
            cmdrsp->params.uart.stopbit = params.stop_bit;
            cmdrsp->params.uart.parity = params.parity;
            cmdrsp->params.uart.flow_ctrl = params.flow_ctrl;
            ext = 1;
            *cmdrsp_size = sizeof(struct tls_hostif_cmd_hdr) + 
                sizeof(HOSTIF_CMDRSP_PARAMS_UART); 
        }
    } else {
        ext = 0;
        *cmdrsp_size = sizeof(struct tls_hostif_cmd_hdr); 
    }

    tls_hostif_fill_cmdrsp_hdr(cmdrsp, HOSTIF_CMD_UART, err, ext);
    *cmdrsp_size += sizeof(struct tls_hostif_cmd_hdr);

    return 0; 
}

int ricmd_atlt_proc(char *buf, u32 length, 
        char *cmdrsp_buf, u32 *cmdrsp_size)
{
    struct tls_hostif_cmdrsp *cmdrsp = (struct tls_hostif_cmdrsp *)cmdrsp_buf;
    struct tls_hostif_cmd *cmd;
    u16 data_len;
    u8 set_opt = 0;
    u8 err = CMD_ERR_OK;
    u8 update_flash = 0;
    u8 *p;

    cmd = (struct tls_hostif_cmd *)buf;

    if (cmd->cmd_hdr.ext & 0x1) {
        if (length == sizeof(struct tls_hostif_cmd_hdr) + 2) {
            if (cmd->cmd_hdr.ext & 0x2)
                update_flash = 1;
            set_opt = 1;
        }  else 
            err = CMD_ERR_INV_PARAMS; 
    } else {
        if (length == sizeof(struct tls_hostif_cmd_hdr)) {
            set_opt = 0;
        } else  
            err = CMD_ERR_INV_PARAMS; 
    }

    if (!err) { 
        if (set_opt) {
            length -= sizeof(struct tls_hostif_cmd_hdr);
            p = (u8 *)(buf + sizeof(struct tls_hostif_cmd_hdr));

            data_len = get_unaligned_be16(p);
            
            err = tls_cmd_set_atlt(data_len, update_flash); 
            if (err)
                err = CMD_ERR_INV_PARAMS;
            tls_hostif_fill_cmdrsp_hdr(cmdrsp, HOSTIF_CMD_ATLT, err, 0);
            *cmdrsp_size = sizeof(struct tls_hostif_cmd_hdr);
        } else {
            tls_cmd_get_atlt(&data_len);
            put_unaligned_be16(data_len, (u8 *)&cmdrsp->params.atlt.length);
            tls_hostif_fill_cmdrsp_hdr(cmdrsp, HOSTIF_CMD_ATLT, 0, 1);
            *cmdrsp_size = sizeof(struct tls_hostif_cmd_hdr) + 2;
        }
    }  else {
        tls_hostif_fill_cmdrsp_hdr(cmdrsp, HOSTIF_CMD_ATLT, err, 0);
        *cmdrsp_size = sizeof(struct tls_hostif_cmd_hdr); 
    }

    return 0; 
}

int ricmd_dns_proc(char *buf, u32 length, 
        char *cmdrsp_buf, u32 *cmdrsp_size)
{
    struct tls_hostif_cmdrsp *cmdrsp = (struct tls_hostif_cmdrsp *)cmdrsp_buf;

    tls_hostif_fill_cmdrsp_hdr(cmdrsp, HOSTIF_CMD_DNS, 0, 0);
    *cmdrsp_size = sizeof(struct tls_hostif_cmd_hdr);

    return 0;

}

int ricmd_ddns_proc(char *buf, u32 length, 
        char *cmdrsp_buf, u32 *cmdrsp_size)
{
    struct tls_hostif_cmdrsp *cmdrsp = (struct tls_hostif_cmdrsp *)cmdrsp_buf;

    tls_hostif_fill_cmdrsp_hdr(cmdrsp, HOSTIF_CMD_DDNS, 0, 0);
    *cmdrsp_size = sizeof(struct tls_hostif_cmd_hdr);

    return 0;

}

int ricmd_upnp_proc(char *buf, u32 length, 
        char *cmdrsp_buf, u32 *cmdrsp_size)
{
    struct tls_hostif_cmdrsp *cmdrsp = (struct tls_hostif_cmdrsp *)cmdrsp_buf;

    tls_hostif_fill_cmdrsp_hdr(cmdrsp, HOSTIF_CMD_UPNP, 0, 0);
    *cmdrsp_size = sizeof(struct tls_hostif_cmd_hdr);

    return 0;

}

int ricmd_dname_proc(char *buf, u32 length, 
        char *cmdrsp_buf, u32 *cmdrsp_size)
{
    struct tls_hostif_cmdrsp *cmdrsp = (struct tls_hostif_cmdrsp *)cmdrsp_buf;

    tls_hostif_fill_cmdrsp_hdr(cmdrsp, HOSTIF_CMD_DNAME, 0, 0);
    *cmdrsp_size = sizeof(struct tls_hostif_cmd_hdr);

    return 0;

}

int ricmd_dbg_proc(char *buf, u32 length, 
        char *cmdrsp_buf, u32 *cmdrsp_size)
{
    struct tls_hostif_cmdrsp *cmdrsp = (struct tls_hostif_cmdrsp *)cmdrsp_buf;

    tls_hostif_fill_cmdrsp_hdr(cmdrsp, HOSTIF_CMD_DBG, 0, 0);
    *cmdrsp_size = sizeof(struct tls_hostif_cmd_hdr);

    return 0;

}

int ricmd_regr_proc(char *buf, u32 length, 
        char *cmdrsp_buf, u32 *cmdrsp_size)
{
    struct tls_hostif_cmdrsp *cmdrsp = (struct tls_hostif_cmdrsp *)cmdrsp_buf;

    tls_hostif_fill_cmdrsp_hdr(cmdrsp, HOSTIF_CMD_REGR, 0, 0);
    *cmdrsp_size = sizeof(struct tls_hostif_cmd_hdr);

    return 0;

}

int ricmd_regw_proc(char *buf, u32 length, 
        char *cmdrsp_buf, u32 *cmdrsp_size)
{
    struct tls_hostif_cmdrsp *cmdrsp = (struct tls_hostif_cmdrsp *)cmdrsp_buf;

    tls_hostif_fill_cmdrsp_hdr(cmdrsp, HOSTIF_CMD_REGW, 0, 0);
    *cmdrsp_size = sizeof(struct tls_hostif_cmd_hdr);

    return 0;

}

int ricmd_rfr_proc(char *buf, u32 length, 
        char *cmdrsp_buf, u32 *cmdrsp_size)
{
    struct tls_hostif_cmdrsp *cmdrsp = (struct tls_hostif_cmdrsp *)cmdrsp_buf;

    tls_hostif_fill_cmdrsp_hdr(cmdrsp, HOSTIF_CMD_RFR, 0, 0);
    *cmdrsp_size = sizeof(struct tls_hostif_cmd_hdr);

    return 0;

}

int ricmd_rfw_proc(char *buf, u32 length, 
        char *cmdrsp_buf, u32 *cmdrsp_size)
{
    struct tls_hostif_cmdrsp *cmdrsp = (struct tls_hostif_cmdrsp *)cmdrsp_buf;

    tls_hostif_fill_cmdrsp_hdr(cmdrsp, HOSTIF_CMD_RFW, 0, 0);
    *cmdrsp_size = sizeof(struct tls_hostif_cmd_hdr);

    return 0;

}

int ricmd_flsr_proc(char *buf, u32 length, 
        char *cmdrsp_buf, u32 *cmdrsp_size)
{
    struct tls_hostif_cmdrsp *cmdrsp = (struct tls_hostif_cmdrsp *)cmdrsp_buf;

    tls_hostif_fill_cmdrsp_hdr(cmdrsp, HOSTIF_CMD_FLSR, 0, 0);
    *cmdrsp_size = sizeof(struct tls_hostif_cmd_hdr);

    return 0;

}

int ricmd_flsw_proc(char *buf, u32 length, 
        char *cmdrsp_buf, u32 *cmdrsp_size)
{
    struct tls_hostif_cmdrsp *cmdrsp = (struct tls_hostif_cmdrsp *)cmdrsp_buf;

    tls_hostif_fill_cmdrsp_hdr(cmdrsp, HOSTIF_CMD_FLSW, 0, 0);
    *cmdrsp_size = sizeof(struct tls_hostif_cmd_hdr);

    return 0;

}

int ricmd_updm_proc(char *buf, u32 length, 
        char *cmdrsp_buf, u32 *cmdrsp_size)
{
    struct tls_hostif_cmdrsp *cmdrsp = (struct tls_hostif_cmdrsp *)cmdrsp_buf;

    tls_hostif_fill_cmdrsp_hdr(cmdrsp, HOSTIF_CMD_UPDM, 0, 0);
    *cmdrsp_size = sizeof(struct tls_hostif_cmd_hdr);

    return 0;

}

int ricmd_updd_proc(char *buf, u32 length, 
        char *cmdrsp_buf, u32 *cmdrsp_size)
{
    struct tls_hostif_cmdrsp *cmdrsp = (struct tls_hostif_cmdrsp *)cmdrsp_buf;

    tls_hostif_fill_cmdrsp_hdr(cmdrsp, HOSTIF_CMD_UPDD, 0, 0);
    *cmdrsp_size = sizeof(struct tls_hostif_cmd_hdr);

    return 0;

}
extern void wm_cmdp_oneshot_status_event(u8 status);
int ricmd_oneshot_proc(char *buf, u32 length, char *cmdrsp_buf, u32 *cmdrsp_size)
{
    struct tls_hostif_cmdrsp *cmdrsp = (struct tls_hostif_cmdrsp *)cmdrsp_buf;
    struct tls_hostif_cmd *cmd;
    u8 err = 0;
	
    cmd = (struct tls_hostif_cmd *)buf;
    if(cmd->cmd_hdr.ext & 0x1) 
	{
		if(tls_cmd_set_oneshot(cmd->params.oneshot.status, 0))
		{
			tls_hostif_fill_cmdrsp_hdr(cmdrsp, HOSTIF_CMD_ONESHOT, CMD_ERR_OPS, 0);
        	*cmdrsp_size = sizeof(struct tls_hostif_cmd_hdr);
		}
		else
		{
			wm_cmdp_oneshot_task_init();
			tls_netif_add_status_event(wm_cmdp_oneshot_status_event);
			tls_hostif_fill_cmdrsp_hdr(cmdrsp, HOSTIF_CMD_ONESHOT, CMD_ERR_OK, 0);
        	*cmdrsp_size = sizeof(struct tls_hostif_cmd_hdr);
		}
    } 
	else 
	{
		tls_hostif_fill_cmdrsp_hdr(cmdrsp, HOSTIF_CMD_ONESHOT, CMD_ERR_INV_PARAMS, 0);
		*cmdrsp_size = sizeof(struct tls_hostif_cmd_hdr);
    } 
    return 0; 
}

int ricmd_httpc_proc(char *buf, u32 length, char *cmdrsp_buf, u32 *cmdrsp_size)
{
	struct tls_hostif_cmdrsp *cmdrsp = (struct tls_hostif_cmdrsp *)cmdrsp_buf;
    struct tls_hostif_cmd *cmd;
    u8 err = 0;
	u8 verb, url_len;
	u16 data_len;
	http_client_msg msg;
	u8 * uri;
	char * sndData = NULL;
	
    cmd = (struct tls_hostif_cmd *)buf;
    if(cmd->cmd_hdr.ext & 0x1) 
	{
		verb = cmd->params.httpc.verb;
		url_len = cmd->params.httpc.url_len;
		data_len = cmd->params.httpc.data_len;
		//printf("###kevin debug ricmd_httpc_proc = %d,%d,%d\r\n", verb, url_len, data_len);
		if(url_len > 255)
		{
			tls_hostif_fill_cmdrsp_hdr(cmdrsp, HOSTIF_CMD_HTTPC, CMD_ERR_NOT_ALLOW, 0);
			*cmdrsp_size = sizeof(struct tls_hostif_cmd_hdr);
			return 0; 
		}
		memset(&msg, 0, sizeof(http_client_msg));
		uri = cmd->params.httpc.url;
		//printf("###kevin debug uri = %s\r\n", uri);
		msg.param.Uri = (char *)uri;
		msg.method = (HTTP_VERB)verb;
		if(VerbGet == verb)
		{
			
		}
		else if((VerbPost == verb) || (VerbPut == verb))
		{
			if(data_len > 512)
			{
				tls_hostif_fill_cmdrsp_hdr(cmdrsp, HOSTIF_CMD_HTTPC, CMD_ERR_NOT_ALLOW, 0);
				*cmdrsp_size = sizeof(struct tls_hostif_cmd_hdr);
				return 0; 
			}
			sndData = uri + url_len;
			//printf("###kevin debug sndData = %s\r\n", sndData);
			msg.dataLen = data_len;
			msg.sendData = sndData;
		}
		else
		{
			tls_hostif_fill_cmdrsp_hdr(cmdrsp, HOSTIF_CMD_HTTPC, CMD_ERR_NOT_ALLOW, 0);
			*cmdrsp_size = sizeof(struct tls_hostif_cmd_hdr);
			return 0; 
		}
		msg.recv_fn = tls_hostif_http_client_recv_callback;
		msg.err_fn = tls_hostif_http_client_err_callback;
		http_client_post(&msg);
        tls_hostif_fill_cmdrsp_hdr(cmdrsp, HOSTIF_CMD_HTTPC, CMD_ERR_OK, 0);
        *cmdrsp_size = sizeof(struct tls_hostif_cmd_hdr);
    } 
	else 
	{
		tls_hostif_fill_cmdrsp_hdr(cmdrsp, HOSTIF_CMD_HTTPC, CMD_ERR_INV_PARAMS, 0);
		*cmdrsp_size = sizeof(struct tls_hostif_cmd_hdr);
    } 
    return 0; 

}

#endif /**/

int ricmd_default_proc(
        char *buf, u32 length, 
        char *cmdrsp_buf, u32 *cmdrsp_size)
{
#if TLS_CONFIG_HOSTIF
    struct tls_hostif_cmdrsp *cmdrsp = (struct tls_hostif_cmdrsp *)cmdrsp_buf;
    struct tls_hostif_cmd *cmd = (struct tls_hostif_cmd *)buf;

    /* if cmd is not suppost, return this cmd and err code  */
    tls_hostif_fill_cmdrsp_hdr(cmdrsp, cmd->cmd_hdr.code, CMD_ERR_INV_PARAMS, 0);
    *cmdrsp_size = sizeof(struct tls_hostif_cmd_hdr);
#endif
    return 0;
}

static struct tls_ricmd_t ri_cmd_tbl[]={
#if TLS_CONFIG_RI_CMD
{HOSTIF_CMD_NOP,         ricmd_nop_proc           },
{HOSTIF_CMD_RESET,       ricmd_reset_proc         },
{HOSTIF_CMD_PS,          ricmd_ps_proc            },
{HOSTIF_CMD_RESET_FLASH, ricmd_reset_flash_proc   },
{HOSTIF_CMD_PMTF,        ricmd_pmtf_proc          },
{HOSTIF_CMD_GPIO,        ricmd_gpio_proc          },
{HOSTIF_CMD_MAC,         ricmd_get_mac_proc       },
{HOSTIF_CMD_VER,         ricmd_ver_proc           },
{HOSTIF_CMD_WJOIN,       ricmd_join_proc          },
{HOSTIF_CMD_WLEAVE,      ricmd_disconnect_proc    },
{HOSTIF_CMD_WSCAN,       ricmd_scan_proc          },
{HOSTIF_CMD_LINK_STATUS, ricmd_link_status_proc   },
{HOSTIF_CMD_WPSST,       ricmd_wpsst_proc         },
{HOSTIF_CMD_SKCT,        ricmd_skct_proc          },
{HOSTIF_CMD_SKSTT,       ricmd_skstt_proc         },
{HOSTIF_CMD_SKCLOSE,     ricmd_skclose_proc       },
{HOSTIF_CMD_SKSDF,       ricmd_sksdf_proc         },
{HOSTIF_CMD_WPRT,        ricmd_wprt_proc          },
{HOSTIF_CMD_SSID,        ricmd_ssid_proc          },
{HOSTIF_CMD_KEY,         ricmd_key_proc           },
{HOSTIF_CMD_ENCRYPT,     ricmd_encrypt_proc       },
{HOSTIF_CMD_BSSID,       ricmd_bssid_proc         },
{HOSTIF_CMD_BRD_SSID,    ricmd_broad_ssid_proc    },
{HOSTIF_CMD_CHNL,        ricmd_chnl_proc          },
{HOSTIF_CMD_WREG,        ricmd_wreg_proc          },
{HOSTIF_CMD_WBGR,        ricmd_wbgr_proc          },
{HOSTIF_CMD_WATC,        ricmd_watc_proc          },
{HOSTIF_CMD_WPSM,        ricmd_wpsm_proc          },
{HOSTIF_CMD_WARM,        ricmd_warm_proc          },
{HOSTIF_CMD_WPS,         ricmd_wps_proc           },
{HOSTIF_CMD_NIP,         ricmd_nip_proc           },
{HOSTIF_CMD_ATM,         ricmd_atm_proc           },
{HOSTIF_CMD_ATRM,        ricmd_atrm_proc          },
{HOSTIF_CMD_AOLM,        NULL			          },
{HOSTIF_CMD_PORTM,       ricmd_portm_proc         },
{HOSTIF_CMD_UART,        ricmd_uart_proc          },
{HOSTIF_CMD_ATLT,        ricmd_atlt_proc          },
{HOSTIF_CMD_DNS,         ricmd_dns_proc           },
{HOSTIF_CMD_DDNS,        ricmd_ddns_proc          },
{HOSTIF_CMD_UPNP,        ricmd_upnp_proc          },
{HOSTIF_CMD_DNAME,       ricmd_dname_proc         },
{HOSTIF_CMD_DBG,         ricmd_dbg_proc           },
{HOSTIF_CMD_REGR,        ricmd_regr_proc          },
{HOSTIF_CMD_REGW,        ricmd_regw_proc          },
{HOSTIF_CMD_RFR,         ricmd_rfr_proc           },
{HOSTIF_CMD_RFW,         ricmd_rfw_proc           },
{HOSTIF_CMD_FLSR,        ricmd_flsr_proc          },
{HOSTIF_CMD_FLSW,        ricmd_flsw_proc          },
{HOSTIF_CMD_UPDM,        ricmd_updm_proc          },
{HOSTIF_CMD_UPDD,        ricmd_updd_proc          },
{HOSTIF_CMD_ONESHOT,     ricmd_oneshot_proc       },
{HOSTIF_CMD_HTTPC,       ricmd_httpc_proc         },
#endif
{-1,                     NULL}
};

int tls_ricmd_exec(char *buf, u32 length, char *cmdrsp_buf, u32 *cmdrsp_size)
{
    struct tls_hostif_cmd *cmd = (struct tls_hostif_cmd *)buf;
    //struct tls_hostif_cmdrsp *cmdrsp = (struct tls_hostif_cmdrsp *)cmdrsp_buf;
    int err = 0;
    int cmdcnt = sizeof(ri_cmd_tbl)/ sizeof(struct tls_ricmd_t);
	int i = 0;
    
    //TLS_DBGPRT_INFO("========>\n");

    int cmd_code = cmd->cmd_hdr.code;

    if ((cmd->cmd_hdr.msg_type != 0x01) || 
            ((cmd->cmd_hdr.ext == 0) && (length != sizeof(struct tls_hostif_cmd_hdr)))) {
        ricmd_default_proc(buf, length, cmdrsp_buf, cmdrsp_size);
        return 0;
    }

 	/*find cmdId*/
	for (i = 0; i< cmdcnt; i++){
		if (cmd_code == ri_cmd_tbl[i].cmdId){
			break;
		}
	}
	if (cmd_code != -1){
		ri_cmd_tbl[i].proc_func(buf, length, cmdrsp_buf, cmdrsp_size);
	}else{
		err = ricmd_default_proc(buf, length, cmdrsp_buf, cmdrsp_size);
	}

    return err; 
}
#endif /*TLS_CONFIG_HOSTIF*/
#endif

