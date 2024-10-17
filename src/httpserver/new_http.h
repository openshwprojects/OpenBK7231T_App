#ifndef _NEW_HTTP_H
#define _NEW_HTTP_H

extern int g_indexAutoRefreshInterval;
extern const char httpHeader[];  // HTTP header
extern const char httpMimeTypeHTML[];              // HTML MIME type
extern const char httpMimeTypeText[];           // TEXT MIME type
extern const char httpMimeTypeJson[];
extern const char httpMimeTypeBinary[];
extern const char httpMimeTypeXML[];

extern const char htmlShortcutIcon[];
extern const char htmlDoctype[];
extern const char htmlHeadMeta[];

extern const char htmlFooterReturnToMainPage[];
extern const char htmlFooterRefreshLink[];
extern const char htmlFooterReturnToCfgOrMainPage[];

extern const char* htmlPinRoleNames[];

extern const char* g_build_str;

extern const char htmlHeadStyle[];
extern const char pageScript[];
extern const char ha_discovery_script[];

#define HTTP_RESPONSE_OK 200
#define HTTP_RESPONSE_NOT_FOUND 404
#define HTTP_RESPONSE_SERVER_ERROR 500

#define MAX_QUERY 16
#define MAX_HEADERS 16
typedef struct http_request_tag {
	char* received; // partial or whole received data, up to 1024
	int receivedLen;
	int receivedLenmax; // sizeof received

	// filled by HTTP_ProcessPacket
	int method;
	char* url;
	int numqueryitems;
	char* querynames[MAX_QUERY];
	char* queryvalues[MAX_QUERY];
	int numheaders;
	char* headers[MAX_HEADERS];
	char* bodystart; /// start start of the body (maybe all of it)
	int bodylen;
	int contentLength;
	int responseCode;

	// used to respond
	char* reply;
	int replylen;
	int replymaxlen;
	int fd;

	// user variables used to build JSON data
	int userCounter;
} http_request_t;


int HTTP_ProcessPacket(http_request_t* request);
void http_setup(http_request_t* request, const char* type);
void http_html_start(http_request_t* request, const char* pagename);
void http_html_end(http_request_t* request);
int poststr(http_request_t* request, const char* str);
void poststr_escaped(http_request_t* request, char* str);
int postany(http_request_t* request, const char* str, int len);
void misc_formatUpTimeString(int totalSeconds, char* o);
// void HTTP_AddBuildFooter(http_request_t *request);
// void HTTP_AddHeader(http_request_t *request);
int http_getRawArg(const char* base, const char* name, char* o, int maxSize);
int http_getArg(const char* base, const char* name, char* o, int maxSize);
int http_getArgInteger(const char* base, const char* name);

// poststr with format - for results LESS THAN 128
int hprintf255(http_request_t* request, const char* fmt, ...);

typedef enum {
	HTTP_ANY = -1,
	HTTP_GET = 0,
	HTTP_PUT = 1,
	HTTP_POST = 2,
	HTTP_OPTIONS = 3
} METHODS;

// callback function for http
typedef int (*http_callback_fn)(http_request_t* request);
// url MUST start with '/'
// urls must be unique (i.e. you can't have /about and /aboutme or /about/me)
int HTTP_RegisterCallback(const char* url, int method, http_callback_fn callback, int auth_required);

int my_strnicmp(const char* a, const char* b, int len);

#endif

