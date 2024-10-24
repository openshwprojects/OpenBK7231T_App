#include <string.h>
#include <assert.h>
#include "wm_include.h"
#include "wm_demo.h"
#include "wm_rtc.h"

#if 0
/**
    NOTE: This demo shows a rtc irq occurs at 00:00:00 on everyday;

*/

#define NTP_UPDATE_PERIOD         (360*HZ)


typedef struct{
    bool alarm_triger_setting;            //rtc irq enabled
    bool ntp_time_updated;                //ntp time updated
    bool ntp_update_frequency;            //ntp update high/low duty
    uint32_t alarm_time;                  //rtc irq occurs time;
    uint32_t alarm_setting_time;          //rtc irq enable time;    
    tls_os_timer_t *ntp_update_timer;
} rtc_s;

static rtc_s * rtc_ptr = NULL;

static void rtc_alarm_irq(void *arg)
{
    struct tm *time_tm = NULL;
    struct tm  rtc_time_tm;
    rtc_s *rtc = (rtc_s *)arg;

    rtc->alarm_triger_setting = false;
    rtc->ntp_time_updated = false;

    tls_get_rtc(&rtc_time_tm);

    printf("ALARM INFO...........................\r\n");
    printf("current rtc time: %04u/%02u/%02u %02u:%02u:%02u\n", rtc_time_tm.tm_year+1900, rtc_time_tm.tm_mon+1, rtc_time_tm.tm_mday, rtc_time_tm.tm_hour, rtc_time_tm.tm_min, rtc_time_tm.tm_sec);

    time_tm = localtime((const time_t *)&rtc->alarm_time);
    printf("alarm rtc time: %04u/%02u/%02u %02u:%02u:%02u\n", time_tm->tm_year+1900, time_tm->tm_mon+1, time_tm->tm_mday, time_tm->tm_hour, time_tm->tm_min, time_tm->tm_sec);

    time_tm = localtime((const time_t *)&rtc->alarm_setting_time);
    printf("alarm rtc setting time: %04u/%02u/%02u %02u:%02u:%02u\n", time_tm->tm_year+1900, time_tm->tm_mon+1, time_tm->tm_mday, time_tm->tm_hour, time_tm->tm_min, time_tm->tm_sec);

    printf("ALARM INFO END...........................\r\n");
}

static void ntp_update_timer_cb(void *ptmr, void *parg)
{
    int ret;
    uint32_t time_now;
    struct tm *time_tm = NULL;
    struct tm  rtc_time_tm;
    
    rtc_s *rtc = (rtc_s *)parg;
    time_now = tls_ntp_client();

    if(time_now != 0)
    {
        time_tm = localtime((const time_t *)&time_now); //switch to local time format
        if(!rtc->ntp_time_updated)
        {
            //rtc->ntp_time_updated = true;
            tls_set_rtc(time_tm);
            printf("update  rtc time: %04u/%02u/%02u %02u:%02u:%02u\n", time_tm->tm_year+1900, time_tm->tm_mon+1, time_tm->tm_mday, time_tm->tm_hour, time_tm->tm_min, time_tm->tm_sec);  
        }else
        {
            printf("current ntp time: %04u/%02u/%02u %02u:%02u:%02u\n", time_tm->tm_year+1900, time_tm->tm_mon+1, time_tm->tm_mday, time_tm->tm_hour, time_tm->tm_min, time_tm->tm_sec);   
        }
    
        if(!rtc->ntp_update_frequency){
            tls_os_timer_change((tls_os_timer_t *)ptmr, NTP_UPDATE_PERIOD);
            rtc->ntp_update_frequency = true;                               //indicate ntp update in low duty
            printf("ntp timer update running low frequency\r\n");
        }

        if(!rtc->alarm_triger_setting)
        {
            //rtc->alarm_triger_setting = true;
            rtc->alarm_setting_time = time_now;

            //configure rtc irq occurs at next day on 00:00:00
            rtc->alarm_time = time_now + 24*3600; //plus one day;
            time_tm = localtime((const time_t *)&rtc->alarm_time);	//switch to local time format
            time_tm->tm_hour = 0;                 //overwritten to 0                
            time_tm->tm_min =  0;                 //overwritten to 0
            time_tm->tm_sec =  0;                 //overwritten to 0  
            rtc->alarm_time = mktime(time_tm);                      //remember the alarm time                 

            if(1)
            {
                tls_rtc_timer_start(time_tm); 
                printf("alarm time setting: %04u/%02u/%02u %02u:%02u:%02u\n", time_tm->tm_year+1900, time_tm->tm_mon+1, time_tm->tm_mday, time_tm->tm_hour, time_tm->tm_min, time_tm->tm_sec);
            }
            
            tls_get_rtc((struct tm *)&rtc_time_tm);
            printf("current rtc time: %04u/%02u/%02u %02u:%02u:%02u\n", rtc_time_tm.tm_year+1900, rtc_time_tm.tm_mon+1, rtc_time_tm.tm_mday, rtc_time_tm.tm_hour, rtc_time_tm.tm_min, rtc_time_tm.tm_sec);
                      
        }  
    }else{
        printf("ntp update failed...\r\n");
        tls_os_timer_change((tls_os_timer_t *)ptmr, NTP_UPDATE_PERIOD/100);
        rtc->ntp_update_frequency = false;                                 //indicate ntp updating in high duty;
        tls_get_rtc((struct tm *)&rtc_time_tm);
        printf("old rtc time: %04u/%02u/%02u %02u:%02u:%02u\n", rtc_time_tm.tm_year+1900, rtc_time_tm.tm_mon+1, rtc_time_tm.tm_mday, rtc_time_tm.tm_hour, rtc_time_tm.tm_min, rtc_time_tm.tm_sec);     
    }
    
}
int rtc_demo(void)
{
    int ret;

    if(rtc_ptr) return 0;

    rtc_ptr = tls_mem_alloc(sizeof(rtc_s));
    assert(rtc_ptr != NULL);

    memset(rtc_ptr, 0, sizeof(rtc_s));
    tls_rtc_isr_register(rtc_alarm_irq, (void*)rtc_ptr);
    ret = tls_os_timer_create(&rtc_ptr->ntp_update_timer,ntp_update_timer_cb,(void*)rtc_ptr, NTP_UPDATE_PERIOD/100, TRUE, (uint8_t *)"ntp_update_timer");

    if(ret == TLS_OS_SUCCESS)
    {
        tls_os_timer_start(rtc_ptr->ntp_update_timer);
        
    }else{
        
        tls_mem_free(rtc_ptr);
        rtc_ptr = NULL;
    }

    return ret;
 
}

int rtc_demo_deinit()
{
    if(rtc_ptr == NULL) return 0;

    tls_os_timer_stop(rtc_ptr->ntp_update_timer);
    tls_os_timer_delete(rtc_ptr->ntp_update_timer);
    rtc_ptr->ntp_update_timer = NULL;
    tls_rtc_timer_stop();
    tls_mem_free(rtc_ptr);
    rtc_ptr = NULL;
}

#else


static void demo_rtc_clock_irq(void *arg)
{
    struct tm tblock;
    tls_get_rtc(&tblock);
    printf("rtc clock, sec=%d,min=%d,hour=%d,mon=%d,year=%d\n", tblock.tm_sec, tblock.tm_min, tblock.tm_hour, tblock.tm_mon, tblock.tm_year);
}


int rtc_demo(void)
{
    struct tm tblock;

    tblock.tm_year = 17;
    tblock.tm_mon = 11;
    tblock.tm_mday = 20;
    tblock.tm_hour = 14;
    tblock.tm_min = 30;
    tblock.tm_sec = 0;
    tls_set_rtc(&tblock);

    tls_rtc_isr_register(demo_rtc_clock_irq, NULL);
    tblock.tm_year = 17;
    tblock.tm_mon = 11;
    tblock.tm_mday = 20;
    tblock.tm_hour = 14;
    tblock.tm_min = 30;
    tblock.tm_sec = 20;
    tls_rtc_timer_start(&tblock);

    while(1)
    {
        tls_os_time_delay(200);
        tls_get_rtc(&tblock);
        printf("rtc cnt, sec=%02d,min=%02d,hour=%02d,mon=%02d,year=%02d\n", tblock.tm_sec, tblock.tm_min, tblock.tm_hour, tblock.tm_mon, tblock.tm_year);
    }

    return WM_SUCCESS;
}
#endif



