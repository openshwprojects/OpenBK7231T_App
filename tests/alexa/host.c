#include "host.h"
int running = 3, values[CHANNEL_MAX], outputs[CHANNEL_MAX];
static char response[32768], packets[8][1024];
static int packetCount, finishes;
int test_fd = 0;
static int (*wemoEvent)(http_request_t*), (*wemoSetup)(http_request_t*);
const char httpMimeTypeJson[] = "application/json", httpMimeTypeXML[] = "text/xml";
void HUE_Init(void);
void WEMO_Init(void);
int HUE_APICall(http_request_t*);
bool DRV_IsRunning(const char *name) { return running & (!strcasecmp(name,"HUE") ? 1 : 2); }
const char *HAL_GetMyIPString(void) { return "192.0.2.10"; }
void WiFI_GetMacAddress(char *mac) { const unsigned char m[] = {0x50,0x12,0x34,0xff,0xff,0xff}; memcpy(mac,m,6); }
const char *CFG_GetDeviceName(void) { return "Switch & \"Salon\""; }
bool CHANNEL_HasLabel(int ch) { return ch == 1; }
const char *CHANNEL_GetLabel(int ch) { (void)ch; return "Lampe \"salon\""; }
int CHANNEL_Get(int ch) { return values[ch]; }
int CHANNEL_GetType(int ch) { return outputs[ch] == 2 ? ChType_Toggle : 0; }
int h_isChannelRelay(int ch) { return outputs[ch] == 1; }
void CHANNEL_Set(int ch,int value,int flags) { (void)flags; values[ch]=value; }
int LED_IsLEDRunning(void) { return 0; }
int LED_GetEnableAll(void) { return 0; }
void CMD_ExecuteCommand(const char *cmd, int flags) { (void)cmd; (void)flags; abort(); }
void http_setup(http_request_t *r,const char *type) { (void)r; (void)type; }
int poststr(http_request_t *r,const char *s) {
 (void)r;
 if (!s) { finishes++; return 0; }
 size_t n=strlen(s); if (strlen(response)+n>=sizeof(response)) abort();
 strcat(response,s); return (int)n;
}
// Match firmware's 255-character formatting limit.
int hprintf255(http_request_t *r,const char *fmt,...) {
 char b[256]; va_list ap; va_start(ap,fmt); vsnprintf(b,sizeof(b),fmt,ap); va_end(ap); return poststr(r,b);
}
void poststr_escapedForJSON(http_request_t *r,char *s) {
 for (;*s;s++) { char b[8]; if (*s=='"'||*s=='\\') { b[0]='\\';b[1]=*s;b[2]=0; }
 else if ((unsigned char)*s<32) snprintf(b,sizeof(b),"\\u%04x",(unsigned char)*s);
 else { b[0]=*s;b[1]=0; } poststr(r,b); }
}
void poststr_escaped(http_request_t *r,char *s) {
 for (;*s;s++) { char b[2]={*s,0}; poststr(r,*s=='&'?"&amp;":*s=='"'?"&quot;":*s=='<'?"&lt;":*s=='>'?"&gt;":b); }
}
void DRV_SSDP_SendReply(struct sockaddr_in *addr,const char *data) {
 (void)addr; if(packetCount>=8||strlen(data)>=1024) abort(); strcpy(packets[packetCount++],data);
}
int HTTP_RegisterCallback(const char *url,int method,int (*fn)(http_request_t*),int auth) {
 (void)method;(void)auth;
 if(!strcmp(url,"/upnp/control/basicevent1")) wemoEvent=fn;
 if(!strcmp(url,"/setup.xml")) wemoSetup=fn;
 return 0;
}
const char *call(int wemo,int method,const char *url,const char *body,int length,int contentLength) {
 http_request_t r={0}; response[0]=0;finishes=0;
 r.fd=test_fd;r.method=method;r.url=(char*)url;r.bodystart=(char*)body;r.bodylen=length;r.contentLength=contentLength;
 if(wemo==1) wemoEvent(&r); else if(wemo==2) wemoSetup(&r); else HUE_APICall(&r);
 if(response[0] && finishes!=1) abort();
 return response;
}
void reset(void) { memset(values,0,sizeof(values));memset(outputs,0,sizeof(outputs));outputs[1]=1;outputs[2]=1;outputs[63]=2;running=3;packetCount=0; }
void init(void) { HUE_Init(); WEMO_Init(); }
const char *packet(int i) { return i<packetCount?packets[i]:NULL; }

void DRV_SSDP_Send_Advert_To(struct sockaddr_in *addr) { DRV_SSDP_SendReply(addr,"OpenBeken fallback"); }
