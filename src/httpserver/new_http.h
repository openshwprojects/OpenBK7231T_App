

extern const char httpHeader[];  // HTTP header
extern const char httpMimeTypeHTML[];              // HTML MIME type
extern const char httpMimeTypeText[];           // TEXT MIME type
extern const char httpMimeTypeJson[];
extern const char htmlHeader[];
extern const char htmlEnd[];
extern const char htmlReturnToMenu[];

extern const char *htmlPinRoleNames[];

#define MAX_QUERY 16 
#define MAX_HEADERS 16
typedef struct http_request_tag {
    char *received; // partial or whole received data, up to 1024
    int receivedLen;

    // filled by HTTP_ProcessPacket
    int method;
    char *url;
    int numqueryitems;
    char *querynames[MAX_QUERY];
    char *queryvalues[MAX_QUERY];
    int numheaders;
    char *headers[MAX_HEADERS];
    char *bodystart; /// start start of the body (maybe all of it)
    int contentLength;

    // used to respond
    char *reply;
    int replylen;
    int replymaxlen;
    int fd;
} http_request_t;


int HTTP_ProcessPacket(http_request_t *request);
void http_setup(http_request_t *request, const char *type);
int poststr(http_request_t *request, const char *str);
void misc_formatUpTimeString(int totalSeconds, char *o);

// poststr with format - for results LESS THAN 128
int hprintf128(http_request_t *request, const char *fmt, ...);

enum {
    HTTP_ANY = -1,
    HTTP_GET = 0,
    HTTP_PUT = 1,
    HTTP_POST = 2,
    HTTP_OPTIONS = 3
} METHODS;

// callback function for http
typedef int (*http_callback_fn)(http_request_t *request);
// url MUST start with '/'
// urls must be unique (i.e. you can't have /about and /aboutme or /about/me)
int HTTP_RegisterCallback( const char *url, int method, http_callback_fn callback);