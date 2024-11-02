/*
 * Copyright (c) 2016-2022 Bouffalolab.
 *
 * This file is part of
 *     *** Bouffalolab Software Dev Kit ***
 *      (see www.bouffalolab.com).
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *   1. Redistributions of source code must retain the above copyright notice,
 *      this list of conditions and the following disclaimer.
 *   2. Redistributions in binary form must reproduce the above copyright notice,
 *      this list of conditions and the following disclaimer in the documentation
 *      and/or other materials provided with the distribution.
 *   3. Neither the name of Bouffalo Lab nor the names of its contributors
 *      may be used to endorse or promote products derived from this software
 *      without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#ifndef __BL_PHY_API_H__
#define __BL_PHY_API_H__

void bl_mfg_phy_init();
void bl_mfg_rf_cal();
void bl_mfg_rx_start();
void bl_mfg_rx_stop();
void bl_mfg_channel_switch(uint8_t chan_no);
int8_t bl_mfg_tx11n_start_raw(uint8_t mcs_n, uint16_t frame_len, uint8_t pwr_dbm);
int8_t bl_mfg_tx11b_start_raw(uint8_t mcs_b, uint16_t frame_len, uint8_t pwr_dbm);
void bl_mfg_tx_stop();
void phy_powroffset_set(int8_t power_offset[14]);

#define RFTLV_TYPE_XTAL_MODE           0X0001
#define RFTLV_TYPE_XTAL                0X0002
#define RFTLV_TYPE_PWR_MODE            0X0003
#define RFTLV_TYPE_PWR_TABLE           0X0004
#define RFTLV_TYPE_PWR_TABLE_11B       0X0005
#define RFTLV_TYPE_PWR_TABLE_11G       0X0006
#define RFTLV_TYPE_PWR_TABLE_11N       0X0007
#define RFTLV_TYPE_PWR_OFFSET          0X0008
#define RFTLV_TYPE_CHAN_DIV_TAB        0X0009
#define RFTLV_TYPE_CHAN_CNT_TAB        0X000A
#define RFTLV_TYPE_LO_FCAL_DIV         0X000B
#define RFTLV_TYPE_EN_TCAL             0X0020
#define RFTLV_TYPE_LINEAR_OR_FOLLOW    0X0021
#define RFTLV_TYPE_TCHANNELS           0X0022
#define RFTLV_TYPE_TCHANNEL_OS         0X0023
#define RFTLV_TYPE_TCHANNEL_OS_LOW     0X0024
#define RFTLV_TYPE_TROOM_OS            0X0025
#define RFTLV_TYPE_PWR_TABLE_BLE       0X0030

#define RFTLV_API_TYPE_XTAL_MODE           RFTLV_TYPE_XTAL_MODE       
#define RFTLV_API_TYPE_XTAL                RFTLV_TYPE_XTAL            
#define RFTLV_API_TYPE_PWR_MODE            RFTLV_TYPE_PWR_MODE        
#define RFTLV_API_TYPE_PWR_TABLE           RFTLV_TYPE_PWR_TABLE       
#define RFTLV_API_TYPE_PWR_TABLE_11B       RFTLV_TYPE_PWR_TABLE_11B   
#define RFTLV_API_TYPE_PWR_TABLE_11G       RFTLV_TYPE_PWR_TABLE_11G   
#define RFTLV_API_TYPE_PWR_TABLE_11N       RFTLV_TYPE_PWR_TABLE_11N   
#define RFTLV_API_TYPE_PWR_OFFSET          RFTLV_TYPE_PWR_OFFSET      
#define RFTLV_API_TYPE_CHAN_DIV_TAB        RFTLV_TYPE_CHAN_DIV_TAB    
#define RFTLV_API_TYPE_CHAN_CNT_TAB        RFTLV_TYPE_CHAN_CNT_TAB    
#define RFTLV_API_TYPE_LO_FCAL_DIV         RFTLV_TYPE_LO_FCAL_DIV     
#define RFTLV_API_TYPE_EN_TCAL             RFTLV_TYPE_EN_TCAL         
#define RFTLV_API_TYPE_LINEAR_OR_FOLLOW    RFTLV_TYPE_LINEAR_OR_FOLLOW
#define RFTLV_API_TYPE_TCHANNELS           RFTLV_TYPE_TCHANNELS       
#define RFTLV_API_TYPE_TCHANNEL_OS         RFTLV_TYPE_TCHANNEL_OS     
#define RFTLV_API_TYPE_TCHANNEL_OS_LOW     RFTLV_TYPE_TCHANNEL_OS_LOW 
#define RFTLV_API_TYPE_TROOM_OS            RFTLV_TYPE_TROOM_OS        
#define RFTLV_API_TYPE_PWR_TABLE_BLE       RFTLV_TYPE_PWR_TABLE_BLE   

/*
 * input:
 *       tlv_addr: rftlv info address
 *
 * return : 1-valid, other-invalid
 */
int rftlv_valid(uint32_t tlv_addr);

/*
 * input:
 *        tlv_addr: rftlv info address
 *        type: type
 *        value_len: value max len
 *        value: point value
 * return : ">0"-success, "<0"-invalid and end, "==0"-invalid and can next
 */
int rftlv_get(uint32_t tlv_addr, uint16_t type, uint32_t value_len, void *value);

#endif
