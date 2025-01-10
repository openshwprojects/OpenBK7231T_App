#include "ota.h"

#if defined(PLATFORM_W600)

//W600 uses OTA functions from its SDK.

#else

#include "../new_common.h"
#include "../new_cfg.h"
#include "typedef.h"
#include "flash_pub.h"
//#include "flash.h"
#include "../logging/logging.h"
#include "../httpclient/http_client.h"
#include "../driver/drv_public.h"
#include "../driver/drv_bl_shared.h"

static unsigned char *sector = (void *)0;
int sectorlen = 0;
unsigned int addr = 0xff000;
#define SECTOR_SIZE 0x1000
static void store_sector(unsigned int addr, unsigned char *data);
extern void flash_protection_op(UINT8 mode,PROTECT_TYPE type);

// from wlan_ui.c
void bk_reboot(void);

// from flash.c
extern UINT32 flash_read(char *user_buf, UINT32 count, UINT32 address);
extern UINT32 flash_write(char *user_buf, UINT32 count, UINT32 address);
extern UINT32 flash_ctrl(UINT32 cmd, void *parm);


int init_ota(unsigned int startaddr){
    flash_init();
	  flash_protection_op(FLASH_XTX_16M_SR_WRITE_ENABLE, FLASH_PROTECT_NONE);
    if (startaddr > 0xff000){
        if (sector){
            addLogAdv(LOG_INFO, LOG_FEATURE_OTA,"aborting OTS, sector already non-null\n");
            return 0;
        }
        sector = os_malloc(SECTOR_SIZE);
        sectorlen = 0;
        addr = startaddr;
        addLogAdv(LOG_INFO, LOG_FEATURE_OTA,"init OTA, startaddr 0x%x\n", startaddr);
        return 1;
    }
    addLogAdv(LOG_INFO, LOG_FEATURE_OTA,"aborting OTA, startaddr 0x%x < 0xff000\n", startaddr);
    return 0;
}

void close_ota(){
    addLogAdv(LOG_INFO, LOG_FEATURE_OTA,"\r\n");
    if (sectorlen){
        addLogAdv(LOG_INFO, LOG_FEATURE_OTA,"close OTA, additional 0x%x FF added \n", SECTOR_SIZE - sectorlen);
        memset(sector+sectorlen, 0xff, SECTOR_SIZE - sectorlen);
        sectorlen = SECTOR_SIZE;
        store_sector(addr, sector);
        addr += 1024;
        sectorlen = 0;
    }
    addLogAdv(LOG_INFO, LOG_FEATURE_OTA,"close OTA, addr 0x%x\n", addr);

    os_free(sector);
    sector = (void *)0;
	  flash_protection_op(FLASH_XTX_16M_SR_WRITE_ENABLE, FLASH_UNPROTECT_LAST_BLOCK);
}

void add_otadata(unsigned char *data, int len)
{
    if (!sector) return;
    //addLogAdv(LOG_INFO, LOG_FEATURE_OTA,"OTA DataRxed start: %02.2x %02.2x len %d\r\n", data[0], data[1], len);
    while (len > 0)
    {
        // force it to sleep...  we MUST have some idle task processing
	    // else task memory doesn't get freed

        if (sectorlen < SECTOR_SIZE)
        {
            int lenstore = SECTOR_SIZE - sectorlen;
            if (lenstore > len) 
                lenstore = len;
            memcpy(sector + sectorlen, data, lenstore);
            data += lenstore;
            len -= lenstore;
            sectorlen += lenstore;
            //addLogAdv(LOG_INFO, LOG_FEATURE_OTA,"OTA sector start: %02.2x %02.2x len %d\r\n", sector[0], sector[1], sectorlen);
        }

        if (sectorlen == SECTOR_SIZE){
            store_sector(addr, sector);
            addr += SECTOR_SIZE;
            sectorlen = 0;
        } else {
            //addLogAdv(LOG_INFO, LOG_FEATURE_OTA,"OTA sectorlen 0x%x not yet 0x%x\n", sectorlen, SECTOR_SIZE);
            rtos_delay_milliseconds(10);
        }
    }
}

static void store_sector(unsigned int addr, unsigned char *data){
    //if (!(addr % 0x4000))
    {
      addLogAdv(LOG_INFO, LOG_FEATURE_OTA,"%x", addr);
    }
    //addLogAdv(LOG_INFO, LOG_FEATURE_OTA,"writing OTA, addr 0x%x\n", addr);
    flash_ctrl(CMD_FLASH_WRITE_ENABLE, (void *)0);
    flash_ctrl(CMD_FLASH_ERASE_SECTOR, &addr);
    flash_ctrl(CMD_FLASH_WRITE_ENABLE, (void *)0);
    flash_write((char *)data , SECTOR_SIZE, addr);
    OTA_IncrementProgress(SECTOR_SIZE);
}

#if ENABLE_SEND_POSTANDGET

httprequest_t httprequest;

int myhttpclientcallback(httprequest_t* request){

  //httpclient_t *client = &request->client;
  httpclient_data_t *client_data = &request->client_data;

  // NOTE: Called from the client thread, beware
  //It is not clear if we can just update total_bytes instead of incrementing. Maintaining previous behavior.
  OTA_SetTotalBytes(OTA_GetTotalBytes() + request->client_data.response_buf_filled);

  switch(request->state){
    case 0: // start
      //init_ota(0xff000);

      init_ota(START_ADR_OF_BK_PARTITION_OTA);
      addLogAdv(LOG_INFO, LOG_FEATURE_OTA,"\r\nmyhttpclientcallback state %d total %d/%d\r\n", request->state, OTA_GetTotalBytes(), request->client_data.response_content_len);
      break;
    case 1: // data
      if (request->client_data.response_buf_filled){
        unsigned char *d = (unsigned char *)request->client_data.response_buf;
        int l = request->client_data.response_buf_filled;
        add_otadata(d, l);
      }
      break;
    case 2: // ended, write any remaining bytes to the sector
      close_ota();
      OTA_ResetProgress();
      addLogAdv(LOG_INFO, LOG_FEATURE_OTA,"\r\nmyhttpclientcallback state %d total %d/%d\r\n", request->state, OTA_GetTotalBytes(), request->client_data.response_content_len);

      addLogAdv(LOG_INFO, LOG_FEATURE_OTA,"Rebooting in 1 seconds...");

      // record this OTA
      CFG_IncrementOTACount();
      // make sure it's saved before reboot
	  CFG_Save_IfThereArePendingChanges();
#if ENABLE_BL_SHARED
      if (DRV_IsMeasuringPower())
      {
        BL09XX_SaveEmeteringStatistics();
      }
#endif
      rtos_delay_milliseconds(1000);
      bk_reboot();
      break;
  }

  //rtos_delay_milliseconds(500);

  if (request->state == 2){
    //os_free(client_data->response_buf);
    client_data->response_buf = (void*)0;
    client_data->response_buf_len = 0;
  }
  //rtos_delay_milliseconds(100);

  return 0;
}

  // NOTE: these MUST persist
// note: url must have a '/' after host, else it can;t parse it..
static char url[256] = "http://raspberrypi:1880/firmware";
static const char *header = "";
static char *content_type = "text/csv";
static char *post_data = "";
#define BUF_SIZE 2048

char *http_buf = (void *)0;

void otarequest(const char *urlin){
  httprequest_t *request = &httprequest;
  if (request->state == 1){
    addLogAdv(LOG_INFO, LOG_FEATURE_OTA,"********************http in progress, not starting another\r\n");
    return;
  }

  strncpy(url, urlin, sizeof(url));

  OTA_SetTotalBytes(0);
  memset(request, 0, sizeof(*request));
  httpclient_t *client = &request->client;
  httpclient_data_t *client_data = &request->client_data;

  if (http_buf == (void *)0){
    http_buf = os_malloc(BUF_SIZE+1);
    if (http_buf == (void *)0) {
        addLogAdv(LOG_INFO, LOG_FEATURE_OTA,"startrequest Malloc failed.\r\n");
        return;
    }
    memset(http_buf, 0, BUF_SIZE);
  }
  client_data->response_buf = http_buf;  //Sets a buffer to store the result.
  client_data->response_buf_len = BUF_SIZE;  //Sets the buffer size.
  HTTPClient_SetCustomHeader(client, header);  //Sets the custom header if needed.
  client_data->post_buf = post_data;  //Sets the user data to be posted.
  client_data->post_buf_len = strlen(post_data);  //Sets the post data length.
  client_data->post_content_type = content_type;  //Sets the content type.
  request->data_callback = &myhttpclientcallback;
  request->port = 80;//HTTP_PORT;
  request->url = url;
  request->method = HTTPCLIENT_GET;
  request->timeout = 10000;
  HTTPClient_Async_SendGeneric(request);
  //+2 Updating ota_status to 0 as before.
  OTA_ResetProgress();
  OTA_IncrementProgress(1);
 }
#else

void otarequest(const char *urlin) {
	addLogAdv(LOG_INFO, LOG_FEATURE_OTA, "");
}
#endif

#endif


/***** SDK independent code from this point. ******/
int ota_status = -1;
int total_bytes = 0;

int ota_progress()
{
  return ota_status;
}

extern void OTA_ResetProgress()
{
  ota_status = -1;
}

extern void OTA_IncrementProgress(int value)
{
  ota_status += value;
}

int OTA_GetTotalBytes()
{
  return total_bytes;
}

extern void OTA_SetTotalBytes(int value)
{
  total_bytes = value;
}

