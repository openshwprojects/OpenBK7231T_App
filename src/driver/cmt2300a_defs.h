/*
 * THE FOLLOWING FIRMWARE IS PROVIDED: (1) "AS IS" WITH NO WARRANTY; AND
 * (2)TO ENABLE ACCESS TO CODING INFORMATION TO GUIDE AND FACILITATE CUSTOMER.
 * CONSEQUENTLY, CMOSTEK SHALL NOT BE HELD LIABLE FOR ANY DIRECT, INDIRECT OR
 * CONSEQUENTIAL DAMAGES WITH RESPECT TO ANY CLAIMS ARISING FROM THE CONTENT
 * OF SUCH FIRMWARE AND/OR THE USE MADE BY CUSTOMERS OF THE CODING INFORMATION
 * CONTAINED HEREIN IN CONNECTION WITH THEIR PRODUCTS.
 *
 * Copyright (C) CMOSTEK SZ.
 */

/*!
 * @file    cmt2300a_defs.h
 * @brief   CMT2300A registers defines
 *
 * @version 1.2
 * @date    Jul 17 2017
 * @author  CMOSTEK R@D
 */

#ifndef __CMT2300A_DEFS_H
#define __CMT2300A_DEFS_H

/* ---------- CMT bank defines ---------- */
#define CMT2300A_CMT_BANK_ADDR           0x00
#define CMT2300A_CMT_BANK_SIZE           12
#define CMT2300A_CUS_CMT1                0x00
#define CMT2300A_CUS_CMT2                0x01
#define CMT2300A_CUS_CMT3                0x02
#define CMT2300A_CUS_CMT4                0x03
#define CMT2300A_CUS_CMT5                0x04
#define CMT2300A_CUS_CMT6                0x05
#define CMT2300A_CUS_CMT7                0x06
#define CMT2300A_CUS_CMT8                0x07
#define CMT2300A_CUS_CMT9                0x08
#define CMT2300A_CUS_CMT10               0x09
#define CMT2300A_CUS_CMT11               0x0A
#define CMT2300A_CUS_RSSI                0x0B

/* ---------- System bank defines ---------- */
#define CMT2300A_SYSTEM_BANK_ADDR        0x0C
#define CMT2300A_SYSTEM_BANK_SIZE        12
#define CMT2300A_CUS_SYS1                0x0C
#define CMT2300A_CUS_SYS2                0x0D
#define CMT2300A_CUS_SYS3                0x0E
#define CMT2300A_CUS_SYS4                0x0F
#define CMT2300A_CUS_SYS5                0x10
#define CMT2300A_CUS_SYS6                0x11
#define CMT2300A_CUS_SYS7                0x12
#define CMT2300A_CUS_SYS8                0x13
#define CMT2300A_CUS_SYS9                0x14
#define CMT2300A_CUS_SYS10               0x15
#define CMT2300A_CUS_SYS11               0x16
#define CMT2300A_CUS_SYS12               0x17

/* ---------- Frequency bank defines ---------- */
#define CMT2300A_FREQUENCY_BANK_ADDR     0x18
#define CMT2300A_FREQUENCY_BANK_SIZE     8
#define CMT2300A_CUS_RF1                 0x18
#define CMT2300A_CUS_RF2                 0x19
#define CMT2300A_CUS_RF3                 0x1A
#define CMT2300A_CUS_RF4                 0x1B
#define CMT2300A_CUS_RF5                 0x1C
#define CMT2300A_CUS_RF6                 0x1D
#define CMT2300A_CUS_RF7                 0x1E
#define CMT2300A_CUS_RF8                 0x1F

/* ---------- Data rate bank defines ---------- */
#define CMT2300A_DATA_RATE_BANK_ADDR     0x20
#define CMT2300A_DATA_RATE_BANK_SIZE     24
#define CMT2300A_CUS_RF9                 0x20
#define CMT2300A_CUS_RF10                0x21
#define CMT2300A_CUS_RF11                0x22
#define CMT2300A_CUS_RF12                0x23
#define CMT2300A_CUS_FSK1                0x24
#define CMT2300A_CUS_FSK2                0x25
#define CMT2300A_CUS_FSK3                0x26
#define CMT2300A_CUS_FSK4                0x27
#define CMT2300A_CUS_FSK5                0x28
#define CMT2300A_CUS_FSK6                0x29
#define CMT2300A_CUS_FSK7                0x2A
#define CMT2300A_CUS_CDR1                0x2B
#define CMT2300A_CUS_CDR2                0x2C
#define CMT2300A_CUS_CDR3                0x2D
#define CMT2300A_CUS_CDR4                0x2E
#define CMT2300A_CUS_AGC1                0x2F
#define CMT2300A_CUS_AGC2                0x30
#define CMT2300A_CUS_AGC3                0x31
#define CMT2300A_CUS_AGC4                0x32
#define CMT2300A_CUS_OOK1                0x33
#define CMT2300A_CUS_OOK2                0x34
#define CMT2300A_CUS_OOK3                0x35
#define CMT2300A_CUS_OOK4                0x36
#define CMT2300A_CUS_OOK5                0x37

/* ---------- Baseband bank defines ---------- */
#define CMT2300A_BASEBAND_BANK_ADDR      0x38
#define CMT2300A_BASEBAND_BANK_SIZE      29
#define CMT2300A_CUS_PKT1                0x38
#define CMT2300A_CUS_PKT2                0x39
#define CMT2300A_CUS_PKT3                0x3A
#define CMT2300A_CUS_PKT4                0x3B
#define CMT2300A_CUS_PKT5                0x3C
#define CMT2300A_CUS_PKT6                0x3D
#define CMT2300A_CUS_PKT7                0x3E
#define CMT2300A_CUS_PKT8                0x3F
#define CMT2300A_CUS_PKT9                0x40
#define CMT2300A_CUS_PKT10               0x41
#define CMT2300A_CUS_PKT11               0x42
#define CMT2300A_CUS_PKT12               0x43
#define CMT2300A_CUS_PKT13               0x44
#define CMT2300A_CUS_PKT14               0x45
#define CMT2300A_CUS_PKT15               0x46
#define CMT2300A_CUS_PKT16               0x47
#define CMT2300A_CUS_PKT17               0x48
#define CMT2300A_CUS_PKT18               0x49
#define CMT2300A_CUS_PKT19               0x4A
#define CMT2300A_CUS_PKT20               0x4B
#define CMT2300A_CUS_PKT21               0x4C
#define CMT2300A_CUS_PKT22               0x4D
#define CMT2300A_CUS_PKT23               0x4E
#define CMT2300A_CUS_PKT24               0x4F
#define CMT2300A_CUS_PKT25               0x50
#define CMT2300A_CUS_PKT26               0x51
#define CMT2300A_CUS_PKT27               0x52
#define CMT2300A_CUS_PKT28               0x53
#define CMT2300A_CUS_PKT29               0x54

/* ---------- Tx bank defines ---------- */
#define CMT2300A_TX_BANK_ADDR            0x55
#define CMT2300A_TX_BANK_SIZE            11
#define CMT2300A_CUS_TX1                 0x55
#define CMT2300A_CUS_TX2                 0x56
#define CMT2300A_CUS_TX3                 0x57
#define CMT2300A_CUS_TX4                 0x58
#define CMT2300A_CUS_TX5                 0x59
#define CMT2300A_CUS_TX6                 0x5A
#define CMT2300A_CUS_TX7                 0x5B
#define CMT2300A_CUS_TX8                 0x5C
#define CMT2300A_CUS_TX9                 0x5D
#define CMT2300A_CUS_TX10                0x5E
#define CMT2300A_CUS_LBD                 0x5F

/* ---------- Control1 bank defines ---------- */
#define CMT2300A_CONTROL1_BANK_ADDR      0x60
#define CMT2300A_CONTROL1_BANK_SIZE      11
#define CMT2300A_CUS_MODE_CTL            0x60
#define CMT2300A_CUS_MODE_STA            0x61
#define CMT2300A_CUS_EN_CTL              0x62
#define CMT2300A_CUS_FREQ_CHNL           0x63
#define CMT2300A_CUS_FREQ_OFS            0x64
#define CMT2300A_CUS_IO_SEL              0x65
#define CMT2300A_CUS_INT1_CTL            0x66
#define CMT2300A_CUS_INT2_CTL            0x67
#define CMT2300A_CUS_INT_EN              0x68
#define CMT2300A_CUS_FIFO_CTL            0x69
#define CMT2300A_CUS_INT_CLR1            0x6A

/* ---------- Control2 bank defines ---------- */
#define CMT2300A_CONTROL2_BANK_ADDR      0x6B
#define CMT2300A_CONTROL2_BANK_SIZE      7
#define CMT2300A_CUS_INT_CLR2            0x6B
#define CMT2300A_CUS_FIFO_CLR            0x6C
#define CMT2300A_CUS_INT_FLAG            0x6D
#define CMT2300A_CUS_FIFO_FLAG           0x6E
#define CMT2300A_CUS_RSSI_CODE           0x6F
#define CMT2300A_CUS_RSSI_DBM            0x70
#define CMT2300A_CUS_LBD_RESULT          0x71


/* ********** CMT2300A_CUS_CMT2 registers ********** */
#define CMT2300A_MASK_PRODUCT_ID         0xFF

/* ********** CMT2300A_CUS_CMT5 registers ********** */
#define CMT2300A_MASK_LMT_CODE           0xC0

/* ********** CMT2300A_CUS_CMT9 registers ********** */
#define CMT2300A_MASK_RSSI_OFFSET_SIGN   0x80
#define CMT2300A_MASK_DIG_CLKDIV         0x1F

/* ********** CMT2300A_CUS_RSSI registers ********** */
#define CMT2300A_MASK_RSSI_OFFSET        0xF8
#define CMT2300A_MASK_RSSI_SLOPE         0x07

/* ********** CMT2300A_CUS_SYS1 registers ********** */
#define CMT2300A_MASK_LMT_VTR            0xC0
#define CMT2300A_MASK_MIXER_BIAS         0x30
#define CMT2300A_MASK_LNA_MODE           0x0C
#define CMT2300A_MASK_LNA_BIAS           0x03

/* ********** CMT2300A_CUS_SYS2 registers ********** */
#define CMT2300A_MASK_LFOSC_RECAL_EN     0x80
#define CMT2300A_MASK_LFOSC_CAL1_EN      0x40
#define CMT2300A_MASK_LFOSC_CAL2_EN      0x20
#define CMT2300A_MASK_RX_TIMER_EN        0x10
#define CMT2300A_MASK_SLEEP_TIMER_EN     0x08
#define CMT2300A_MASK_TX_DC_EN           0x04
#define CMT2300A_MASK_RX_DC_EN           0x02
#define CMT2300A_MASK_DC_PAUSE           0x01

/* ********** CMT2300A_CUS_SYS3 registers ********** */
#define CMT2300A_MASK_SLEEP_BYPASS_EN    0x80
#define CMT2300A_MASK_XTAL_STB_TIME      0x70
#define CMT2300A_MASK_TX_EXIT_STATE      0x0C
#define CMT2300A_MASK_RX_EXIT_STATE      0x03

/* ********** CMT2300A_CUS_SYS4 registers ********** */
#define CMT2300A_MASK_SLEEP_TIMER_M_7_0  0xFF

/* ********** CMT2300A_CUS_SYS5 registers ********** */
#define CMT2300A_MASK_SLEEP_TIMER_M_10_8 0x70
#define CMT2300A_MASK_SLEEP_TIMER_R      0x0F

/* ********** CMT2300A_CUS_SYS6 registers ********** */
#define CMT2300A_MASK_RX_TIMER_T1_M_7_0  0xFF

/* ********** CMT2300A_CUS_SYS7 registers ********** */
#define CMT2300A_MASK_RX_TIMER_T1_M_10_8 0x70
#define CMT2300A_MASK_RX_TIMER_T1_R      0x0F

/* ********** CMT2300A_CUS_SYS8 registers ********** */
#define CMT2300A_MASK_RX_TIMER_T2_M_7_0  0xFF

/* ********** CMT2300A_CUS_SYS9 registers ********** */
#define CMT2300A_MASK_RX_TIMER_T2_M_10_8 0x70
#define CMT2300A_MASK_RX_TIMER_T2_R      0x0F

/* ********** CMT2300A_CUS_SYS10 registers ********** */
#define CMT2300A_MASK_COL_DET_EN         0x80
#define CMT2300A_MASK_COL_OFS_SEL        0x40
#define CMT2300A_MASK_RX_AUTO_EXIT_DIS   0x20
#define CMT2300A_MASK_DOUT_MUTE          0x10
#define CMT2300A_MASK_RX_EXTEND_MODE     0x0F

/* ********** CMT2300A_CUS_SYS11 registers ********** */
#define CMT2300A_MASK_PJD_TH_SEL         0x80
#define CMT2300A_MASK_CCA_INT_SEL        0x60
#define CMT2300A_MASK_RSSI_DET_SEL       0x18
#define CMT2300A_MASK_RSSI_AVG_MODE      0x07

/* ********** CMT2300A_CUS_SYS12 registers ********** */
#define CMT2300A_MASK_PJD_WIN_SEL        0xC0
#define CMT2300A_MASK_CLKOUT_EN          0x20
#define CMT2300A_MASK_CLKOUT_DIV         0x1F

/* ********** CMT2300A_CUS_RF1 registers ********** */
#define CMT2300A_MASK_FREQ_RX_N          0xFF

/* ********** CMT2300A_CUS_RF2 registers ********** */
#define CMT2300A_MASK_FREQ_RX_K_7_0      0xFF

/* ********** CMT2300A_CUS_RF3 registers ********** */
#define CMT2300A_MASK_FREQ_RX_K_15_8     0xFF

/* ********** CMT2300A_CUS_RF4 registers ********** */
#define CMT2300A_MASK_FREQ_PALDO_SEL     0x80
#define CMT2300A_MASK_FREQ_DIVX_CODE     0x70
#define CMT2300A_MASK_FREQ_RX_K_19_16    0x0F

/* ********** CMT2300A_CUS_RF5 registers ********** */
#define CMT2300A_MASK_FREQ_TX_N          0xFF

/* ********** CMT2300A_CUS_RF6 registers ********** */
#define CMT2300A_MASK_FREQ_TX_K_7_0      0xFF

/* ********** CMT2300A_CUS_RF7 registers ********** */
#define CMT2300A_MASK_FREQ_TX_K_15_8     0xFF

/* ********** CMT2300A_CUS_RF8 registers ********** */
#define CMT2300A_MASK_FSK_SWT            0x80
#define CMT2300A_MASK_FREQ_VCO_BANK      0x70
#define CMT2300A_MASK_FREQ_TX_K_19_16    0x0F

/* ********** CMT2300A_CUS_PKT1 registers ********** */
#define CMT2300A_MASK_RX_PREAM_SIZE      0xF8
#define CMT2300A_MASK_PREAM_LENG_UNIT    0x04
#define CMT2300A_MASK_DATA_MODE          0x03
/* CMT2300A_MASK_PREAM_LENG_UNIT options */
#define CMT2300A_PREAM_LENG_UNIT_8_BITS  0x00
#define CMT2300A_PREAM_LENG_UNIT_4_BITS  0x04
/* CMT2300A_MASK_DATA_MODE options */
#define CMT2300A_DATA_MODE_DIRECT        0x00
#define CMT2300A_DATA_MODE_PACKET        0x02

/* ********** CMT2300A_CUS_PKT2 registers ********** */
#define CMT2300A_MASK_TX_PREAM_SIZE_7_0  0xFF

/* ********** CMT2300A_CUS_PKT3 registers ********** */
#define CMT2300A_MASK_TX_PREAM_SIZE_15_8 0xFF

/* ********** CMT2300A_CUS_PKT4 registers ********** */
#define CMT2300A_MASK_PREAM_VALUE        0xFF

/* ********** CMT2300A_CUS_PKT5 registers ********** */
#define CMT2300A_MASK_SYNC_TOL           0x70
#define CMT2300A_MASK_SYNC_SIZE          0x0E
#define CMT2300A_MASK_SYNC_MAN_EN        0x01

/* ********** CMT2300A_CUS_PKT6 registers ********** */
#define CMT2300A_MASK_SYNC_VALUE_7_0     0xFF

/* ********** CMT2300A_CUS_PKT7 registers ********** */
#define CMT2300A_MASK_SYNC_VALUE_15_8    0xFF

/* ********** CMT2300A_CUS_PKT8 registers ********** */
#define CMT2300A_MASK_SYNC_VALUE_23_16   0xFF

/* ********** CMT2300A_CUS_PKT9 registers ********** */
#define CMT2300A_MASK_SYNC_VALUE_31_24   0xFF

/* ********** CMT2300A_CUS_PKT10 registers ********** */
#define CMT2300A_MASK_SYNC_VALUE_39_32   0xFF

/* ********** CMT2300A_CUS_PKT11 registers ********** */
#define CMT2300A_MASK_SYNC_VALUE_47_40   0xFF

/* ********** CMT2300A_CUS_PKT12 registers ********** */
#define CMT2300A_MASK_SYNC_VALUE_55_48   0xFF

/* ********** CMT2300A_CUS_PKT13 registers ********** */
#define CMT2300A_MASK_SYNC_VALUE_63_56   0xFF

/* ********** CMT2300A_CUS_PKT14 registers ********** */
#define CMT2300A_MASK_PAYLOAD_LENG_10_8  0x70
#define CMT2300A_MASK_AUTO_ACK_EN        0x08
#define CMT2300A_MASK_NODE_LENG_POS_SEL  0x04
#define CMT2300A_MASK_PAYLOAD_BIT_ORDER  0x02
#define CMT2300A_MASK_PKT_TYPE           0x01
/* CMT2300A_MASK_NODE_LENG_POS_SEL options */
#define CMT2300A_NODE_LENG_FIRST_NODE    0x00
#define CMT2300A_NODE_LENG_FIRST_LENGTH  0x04
/* CMT2300A_MASK_PAYLOAD_BIT_ORDER options */
#define CMT2300A_PAYLOAD_BIT_ORDER_MSB   0x00
#define CMT2300A_PAYLOAD_BIT_ORDER_LSB   0x02
/* CMT2300A_MASK_PKT_TYPE options */
#define CMT2300A_PKT_TYPE_FIXED          0x00
#define CMT2300A_PKT_TYPE_VARIABLE       0x01

/* ********** CMT2300A_CUS_PKT15 registers ********** */
#define CMT2300A_MASK_PAYLOAD_LENG_7_0   0xFF

/* ********** CMT2300A_CUS_PKT16 registers ********** */
#define CMT2300A_MASK_NODE_FREE_EN       0x20
#define CMT2300A_MASK_NODE_ERR_MASK      0x10
#define CMT2300A_MASK_NODE_SIZE          0x0C
#define CMT2300A_MASK_NODE_DET_MODE      0x03
/* CMT2300A_MASK_NODE_DET_MODE options */
#define CMT2300A_NODE_DET_NODE           0x00
#define CMT2300A_NODE_DET_VALUE          0x01
#define CMT2300A_NODE_DET_VALUE_0        0x02
#define CMT2300A_NODE_DET_VALUE_0_1      0x03

/* ********** CMT2300A_CUS_PKT17 registers ********** */
#define CMT2300A_MASK_NODE_VALUE_7_0     0xFF

/* ********** CMT2300A_CUS_PKT18 registers ********** */
#define CMT2300A_MASK_NODE_VALUE_15_8    0xFF

/* ********** CMT2300A_CUS_PKT19 registers ********** */
#define CMT2300A_MASK_NODE_VALUE_23_16   0xFF

/* ********** CMT2300A_CUS_PKT20 registers ********** */
#define CMT2300A_MASK_NODE_VALUE_31_24   0xFF

/* ********** CMT2300A_CUS_PKT21 registers ********** */
#define CMT2300A_MASK_FEC_TYPE           0x80
#define CMT2300A_MASK_FEC_EN             0x40
#define CMT2300A_MASK_CRC_BYTE_SWAP      0x20
#define CMT2300A_MASK_CRC_BIT_INV        0x10
#define CMT2300A_MASK_CRC_RANGE          0x08
#define CMT2300A_MASK_CRC_TYPE           0x06
#define CMT2300A_MASK_CRC_EN             0x01
/* CMT2300A_MASK_CRC_BYTE_SWAP options */
#define CMT2300A_CRC_ORDER_HBYTE         0x00
#define CMT2300A_CRC_ORDER_LBYTE         0x20
/* CMT2300A_MASK_CRC_RANGE options */
#define CMT2300A_CRC_RANGE_PAYLOAD       0x00
#define CMT2300A_CRC_RANGE_DATA          0x08
/* CMT2300A_MASK_CRC_TYPE options */
#define CMT2300A_CRC_TYPE_CCITT16        0x00
#define CMT2300A_CRC_TYPE_IBM16          0x02
#define CMT2300A_CRC_TYPE_ITU16          0x04

/* ********** CMT2300A_CUS_PKT22 registers ********** */
#define CMT2300A_MASK_CRC_SEED_7_0       0xFF

/* ********** CMT2300A_CUS_PKT23 registers ********** */
#define CMT2300A_MASK_CRC_SEED_15_8      0xFF

/* ********** CMT2300A_CUS_PKT24 registers ********** */
#define CMT2300A_MASK_CRC_BIT_ORDER      0x80
#define CMT2300A_MASK_WHITEN_SEED_8_8    0x40
#define CMT2300A_MASK_WHITEN_SEED_TYPE   0x20
#define CMT2300A_MASK_WHITEN_TYPE        0x18
#define CMT2300A_MASK_WHITEN_EN          0x04
#define CMT2300A_MASK_MANCH_TYPE         0x02
#define CMT2300A_MASK_MANCH_EN           0x01
/* CMT2300A_MASK_CRC_BIT_ORDER options */
#define CMT2300A_CRC_BIT_ORDER_MSB       0x00
#define CMT2300A_CRC_BIT_ORDER_LSB       0x80
/* CMT2300A_MASK_WHITEN_SEED_TYPE options */
#define CMT2300A_WHITEN_SEED_TYPE_1      0x00
#define CMT2300A_WHITEN_SEED_TYPE_2      0x20
/* CMT2300A_MASK_WHITEN_TYPE options */
#define CMT2300A_WHITEN_TYPE_PN9_CCITT   0x00
#define CMT2300A_WHITEN_TYPE_PN9_IBM     0x08
#define CMT2300A_WHITEN_TYPE_PN7         0x10
/* CMT2300A_MASK_MANCH_TYPE options */
#define CMT2300A_MANCH_TYPE_ONE_01       0x00
#define CMT2300A_MANCH_TYPE_ONE_10       0x02

/* ********** CMT2300A_CUS_PKT25 registers ********** */
#define CMT2300A_MASK_WHITEN_SEED_7_0    0xFF

/* ********** CMT2300A_CUS_PKT26 registers ********** */
#define CMT2300A_MASK_TX_PREFIX_TYPE     0x03

/* ********** CMT2300A_CUS_PKT27 registers ********** */
#define CMT2300A_MASK_TX_PKT_NUM         0xFF

/* ********** CMT2300A_CUS_PKT28 registers ********** */
#define CMT2300A_MASK_TX_PKT_GAP         0xFF

/* ********** CMT2300A_CUS_PKT29 registers ********** */
#define CMT2300A_MASK_FIFO_AUTO_RES_EN   0x80
#define CMT2300A_MASK_FIFO_TH            0x7F

/* ********** CMT2300A_CUS_MODE_CTL registers ********** */
#define CMT2300A_MASK_CHIP_MODE_SWT      0xFF
/* CMT2300A_MASK_CHIP_MODE_SWT options */
#define CMT2300A_GO_EEPROM               0x01
#define CMT2300A_GO_STBY                 0x02
#define CMT2300A_GO_RFS                  0x04
#define CMT2300A_GO_RX                   0x08
#define CMT2300A_GO_SLEEP                0x10
#define CMT2300A_GO_TFS                  0x20
#define CMT2300A_GO_TX                   0x40
#define CMT2300A_GO_SWITCH               0x80

/* ********** CMT2300A_CUS_MODE_STA registers ********** */
#define CMT2300A_MASK_RSTN_IN_EN         0x20
#define CMT2300A_MASK_CFG_RETAIN         0x10
#define CMT2300A_MASK_CHIP_MODE_STA      0x0F
/* CMT2300A_MASK_CHIP_MODE_STA options */
#define CMT2300A_STA_IDLE                0x00
#define CMT2300A_STA_SLEEP               0x01
#define CMT2300A_STA_STBY                0x02
#define CMT2300A_STA_RFS                 0x03
#define CMT2300A_STA_TFS                 0x04
#define CMT2300A_STA_RX                  0x05
#define CMT2300A_STA_TX                  0x06
#define CMT2300A_STA_EEPROM              0x07
#define CMT2300A_STA_ERROR               0x08
#define CMT2300A_STA_CAL                 0x09

/* ********** CMT2300A_CUS_EN_CTL registers ********** */
#define CMT2300A_MASK_LOCKING_EN         0x20

/* ********** CMT2300A_CUS_FREQ_CHNL registers ********** */
#define CMT2300A_MASK_FH_CHANNEL         0xFF

/* ********** CMT2300A_CUS_FREQ_OFS registers ********** */
#define CMT2300A_MASK_FH_OFFSET          0xFF

/* ********** CMT2300A_CUS_IO_SEL registers ********** */
#define CMT2300A_MASK_GPIO4_SEL          0xC0
#define CMT2300A_MASK_GPIO3_SEL          0x30
#define CMT2300A_MASK_GPIO2_SEL          0x0C
#define CMT2300A_MASK_GPIO1_SEL          0x03
/* CMT2300A_MASK_GPIO4_SEL options */
#define CMT2300A_GPIO4_SEL_RSTIN         0x00
#define CMT2300A_GPIO4_SEL_INT1          0x40
#define CMT2300A_GPIO4_SEL_DOUT          0x80
#define CMT2300A_GPIO4_SEL_DCLK          0xC0
/* CMT2300A_MASK_GPIO3_SEL options */
#define CMT2300A_GPIO3_SEL_CLKO          0x00
#define CMT2300A_GPIO3_SEL_DOUT          0x10
#define CMT2300A_GPIO3_SEL_DIN           0x10
#define CMT2300A_GPIO3_SEL_INT2          0x20
#define CMT2300A_GPIO3_SEL_DCLK          0x30
/* CMT2300A_MASK_GPIO2_SEL options */
#define CMT2300A_GPIO2_SEL_INT1          0x00
#define CMT2300A_GPIO2_SEL_INT2          0x04
#define CMT2300A_GPIO2_SEL_DOUT          0x08
#define CMT2300A_GPIO2_SEL_DIN           0x08
#define CMT2300A_GPIO2_SEL_DCLK          0x0C
/* CMT2300A_MASK_GPIO1_SEL options */
#define CMT2300A_GPIO1_SEL_DOUT          0x00
#define CMT2300A_GPIO1_SEL_DIN           0x00
#define CMT2300A_GPIO1_SEL_INT1          0x01
#define CMT2300A_GPIO1_SEL_INT2          0x02
#define CMT2300A_GPIO1_SEL_DCLK          0x03

/* ********** CMT2300A_CUS_INT1_CTL registers ********** */
#define CMT2300A_MASK_RF_SWT1_EN         0x80
#define CMT2300A_MASK_RF_SWT2_EN         0x40
#define CMT2300A_MASK_INT_POLAR          0x20
#define CMT2300A_MASK_INT1_SEL           0x1F
/* CMT2300A_MASK_INT_POLAR options */
#define CMT2300A_INT_POLAR_SEL_0         0x00
#define CMT2300A_INT_POLAR_SEL_1         0x20
/* CMT2300A_MASK_INT1_SEL options */
#define CMT2300A_INT_SEL_RX_ACTIVE       0x00
#define CMT2300A_INT_SEL_TX_ACTIVE       0x01
#define CMT2300A_INT_SEL_RSSI_VLD        0x02
#define CMT2300A_INT_SEL_PREAM_OK        0x03
#define CMT2300A_INT_SEL_SYNC_OK         0x04
#define CMT2300A_INT_SEL_NODE_OK         0x05
#define CMT2300A_INT_SEL_CRC_OK          0x06
#define CMT2300A_INT_SEL_PKT_OK          0x07
#define CMT2300A_INT_SEL_SL_TMO          0x08
#define CMT2300A_INT_SEL_RX_TMO          0x09
#define CMT2300A_INT_SEL_TX_DONE         0x0A
#define CMT2300A_INT_SEL_RX_FIFO_NMTY    0x0B
#define CMT2300A_INT_SEL_RX_FIFO_TH      0x0C
#define CMT2300A_INT_SEL_RX_FIFO_FULL    0x0D
#define CMT2300A_INT_SEL_RX_FIFO_WBYTE   0x0E
#define CMT2300A_INT_SEL_RX_FIFO_OVF     0x0F
#define CMT2300A_INT_SEL_TX_FIFO_NMTY    0x10
#define CMT2300A_INT_SEL_TX_FIFO_TH      0x11
#define CMT2300A_INT_SEL_TX_FIFO_FULL    0x12
#define CMT2300A_INT_SEL_STATE_IS_STBY   0x13
#define CMT2300A_INT_SEL_STATE_IS_FS     0x14
#define CMT2300A_INT_SEL_STATE_IS_RX     0x15
#define CMT2300A_INT_SEL_STATE_IS_TX     0x16
#define CMT2300A_INT_SEL_LED             0x17
#define CMT2300A_INT_SEL_TRX_ACTIVE      0x18
#define CMT2300A_INT_SEL_PKT_DONE        0x19

/* ********** CMT2300A_CUS_INT2_CTL registers ********** */
#define CMT2300A_MASK_LFOSC_OUT_EN       0x40
#define CMT2300A_MASK_TX_DIN_INV         0x20
#define CMT2300A_MASK_INT2_SEL           0x1F

/* ********** CMT2300A_CUS_INT_EN registers ********** */
#define CMT2300A_MASK_SL_TMO_EN          0x80
#define CMT2300A_MASK_RX_TMO_EN          0x40
#define CMT2300A_MASK_TX_DONE_EN         0x20
#define CMT2300A_MASK_PREAM_OK_EN        0x10
#define CMT2300A_MASK_SYNC_OK_EN         0x08
#define CMT2300A_MASK_NODE_OK_EN         0x04
#define CMT2300A_MASK_CRC_OK_EN          0x02
#define CMT2300A_MASK_PKT_DONE_EN        0x01

/* ********** CMT2300A_CUS_FIFO_CTL registers ********** */
#define CMT2300A_MASK_TX_DIN_EN          0x80
#define CMT2300A_MASK_TX_DIN_SEL         0x60
#define CMT2300A_MASK_FIFO_AUTO_CLR_DIS  0x10
#define CMT2300A_MASK_FIFO_TX_RD_EN      0x08
#define CMT2300A_MASK_FIFO_RX_TX_SEL     0x04
#define CMT2300A_MASK_FIFO_MERGE_EN      0x02
#define CMT2300A_MASK_SPI_FIFO_RD_WR_SEL 0x01
/* CMT2300A_MASK_TX_DIN_SEL options */
#define CMT2300A_TX_DIN_SEL_GPIO1        0x00
#define CMT2300A_TX_DIN_SEL_GPIO2        0x20
#define CMT2300A_TX_DIN_SEL_GPIO3        0x40

/* ********** CMT2300A_CUS_INT_CLR1 registers ********** */
#define CMT2300A_MASK_SL_TMO_FLG         0x20
#define CMT2300A_MASK_RX_TMO_FLG         0x10
#define CMT2300A_MASK_TX_DONE_FLG        0x08
#define CMT2300A_MASK_TX_DONE_CLR        0x04
#define CMT2300A_MASK_SL_TMO_CLR         0x02
#define CMT2300A_MASK_RX_TMO_CLR         0x01

/* ********** CMT2300A_CUS_INT_CLR2 registers ********** */
#define CMT2300A_MASK_LBD_CLR            0x20
#define CMT2300A_MASK_PREAM_OK_CLR       0x10
#define CMT2300A_MASK_SYNC_OK_CLR        0x08
#define CMT2300A_MASK_NODE_OK_CLR        0x04
#define CMT2300A_MASK_CRC_OK_CLR         0x02
#define CMT2300A_MASK_PKT_DONE_CLR       0x01

/* ********** CMT2300A_CUS_FIFO_CLR registers ********** */
#define CMT2300A_MASK_FIFO_RESTORE       0x04
#define CMT2300A_MASK_FIFO_CLR_RX        0x02
#define CMT2300A_MASK_FIFO_CLR_TX        0x01

/* ********** CMT2300A_CUS_INT_FLAG registers ********** */
#define CMT2300A_MASK_LBD_FLG            0x80
#define CMT2300A_MASK_COL_ERR_FLG        0x40
#define CMT2300A_MASK_PKT_ERR_FLG        0x20
#define CMT2300A_MASK_PREAM_OK_FLG       0x10
#define CMT2300A_MASK_SYNC_OK_FLG        0x08
#define CMT2300A_MASK_NODE_OK_FLG        0x04
#define CMT2300A_MASK_CRC_OK_FLG         0x02
#define CMT2300A_MASK_PKT_OK_FLG         0x01

/* ********** CMT2300A_CUS_FIFO_FLAG registers ********** */
#define CMT2300A_MASK_RX_FIFO_FULL_FLG   0x40
#define CMT2300A_MASK_RX_FIFO_NMTY_FLG   0x20
#define CMT2300A_MASK_RX_FIFO_TH_FLG     0x10
#define CMT2300A_MASK_RX_FIFO_OVF_FLG    0x08
#define CMT2300A_MASK_TX_FIFO_FULL_FLG   0x04
#define CMT2300A_MASK_TX_FIFO_NMTY_FLG   0x02
#define CMT2300A_MASK_TX_FIFO_TH_FLG     0x01

/* ********** CMT2300A_CUS_RSSI_CODE registers ********** */
#define CMT2300A_MASK_RSSI_CODE          0xFF

/* ********** CMT2300A_CUS_RSSI_DBM registers ********** */
#define CMT2300A_MASK_RSSI_DBM           0xFF

/* ********** CMT2300A_CUS_LBD_RESULT registers ********** */
#define CMT2300A_MASK_LBD_RESULT         0xFF


#endif
