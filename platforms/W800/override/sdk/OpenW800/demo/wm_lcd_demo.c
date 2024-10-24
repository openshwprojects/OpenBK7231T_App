/**************************************************************************//**
 * @file     wm_lcd.c
 * @author
 * @version  
 * @date  
 * @brief
 *
 * Copyright (c) 2014 Winner Microelectronics Co., Ltd. All rights reserved.
 *****************************************************************************/
#include "wm_include.h"

#if DEMO_LCD
#include "wm_osal.h"
#include "wm_lcd.h"
#include "wm_io.h"
#include "wm_pmu.h"

/*********************************************************
  Available COM and SEGMENT

  COM00:PB_25  COM01:PB_21  COM02:PB_22  COM03:PB_27  COM04:PB_28  COM05:PB_29  COM06:PB_30  COM07:PB_31

  SEG00:PB_23  SEG01:PB_26  SEG02:PB_24  SEG03:PA_07  SEG04:PA_08  SEG05:PA_09  SEG06:PA_10  SEG07:PA_11
  SEG08:PA_12  SEG09:PA_13  SEG10:PA_14  SEG11:PA_15  SEG12:PB_00  SEG13:PB_01  SEG14:PB_02  SEG15:PB_03
  SEG16:PB_04  SEG17:PB_05  SEG18:PB_06  SEG19:PB_07  SEG20:PB_08  SEG21:PB_09  SEG22:PB_10  SEG23:PB_11
  SEG24:PB_12  SEG25:PB_13  SEG26:PB_14  SEG27:PB_15  SEG28:PB_16  SEG29:PB_17  SEG30:PB_18  SEG31:PA_06
 ********************************************************/
/*test lcd output after cfg lcd and make lcd pin output fixed state*/
void lcd_test(void)
{
	int i,j;
	tls_lcd_options_t lcd_opts = {
	    true,
	    BIAS_ONEFOURTH,
	    DUTY_ONEEIGHTH,
	    VLCD31,
	    4,
	    60,
	};
	/* COM 0-3 */
	tls_io_cfg_set(WM_IO_PB_25, WM_IO_OPTION6);
	tls_io_cfg_set(WM_IO_PB_21, WM_IO_OPTION6);
	tls_io_cfg_set(WM_IO_PB_22, WM_IO_OPTION6);
	tls_io_cfg_set(WM_IO_PB_27, WM_IO_OPTION6);

	/* SEG 0-5 */
	tls_io_cfg_set(WM_IO_PB_23, WM_IO_OPTION6);
	tls_io_cfg_set(WM_IO_PB_26, WM_IO_OPTION6);
	tls_io_cfg_set(WM_IO_PB_24, WM_IO_OPTION6);
	tls_io_cfg_set(WM_IO_PA_07, WM_IO_OPTION6);
	tls_io_cfg_set(WM_IO_PA_08, WM_IO_OPTION6);
	tls_io_cfg_set(WM_IO_PA_09, WM_IO_OPTION6);	
	tls_open_peripheral_clock(TLS_PERIPHERAL_TYPE_LCD);

	/*enable output valid*/
	tls_reg_write32(HR_LCD_COM_EN, 0xF);
	tls_reg_write32(HR_LCD_SEG_EN, 0x3F);

    tls_lcd_init(&lcd_opts);

	while(1)
	{
		for(i=0; i<4; i++)
		{
			for(j=0; j<6; j++)
			{
				tls_lcd_seg_set(i, j, 1);
				tls_os_time_delay(500);
				printf("%d %d %d\n", i, j, 1);
			}
		}
		
		for(i=0; i<4; i++)
		{
			for(j=0; j<6; j++)
			{
				tls_lcd_seg_set(i, j, 0);
				tls_os_time_delay(500);
				printf("%d %d %d\n", i, j, 0);
			}
		}
	}
}
#endif

