#ifndef _h_MATRIXSSLAPP
#define _h_MATRIXSSLAPP

#include "wm_demo.h"

#if DEMO_SSL_SERVER

#ifdef __cplusplus
extern "C" {
#endif
/******************************************************************************/

#include <errno.h>			/* Defines EWOULDBLOCK, etc. */
#include <stdlib.h>			/* Defines malloc, exit, etc. */
#include "wm_ssl_server.h"
	
/******************************************************************************/
/*
	 Platform independent socket defines for convenience
 */
#ifndef SOCKET
typedef int32 SOCKET;
#endif
#ifndef INVALID_SOCKET
#define INVALID_SOCKET	-1
#endif
	
#define SOCKET_ERRNO	errno
	
/******************************************************************************/
/*
	Configuration Options
*/
#define HTTPS_PORT		4433	/* Port to run the server/client on */

/******************************************************************************/

#if !TLS_CONFIG_USE_MBEDTLS
/*
	Protocol specific defines
 */
/* Maximum size of parseable http element. In this case, a HTTP header line. */
#define HTTPS_BUFFER_MAX 256	
	
/* Return codes from http parsing routine */
#define HTTPS_COMPLETE	1	/* Full request/response parsed */
#define HTTPS_PARTIAL	0	/* Only a partial request/response was received */
#define HTTPS_ERROR		MATRIXSSL_ERROR	/* Invalid/unsupported HTTP syntax */

typedef struct {
	DLListEntry		List;
	tls_ssl_t			*ssl;
	SOCKET			fd;
	psTime_t		time;		/* Last time there was activity */
	uint32			timeout;	/* in milliseconds*/
	uint32			flags;
	unsigned char	*parsebuf;		/* Partial data */
	uint32			parsebuflen;
	uint32			bytes_received;
	uint32			bytes_requested;
	uint32			bytes_sent;
} httpConn_t;

static int32 httpBasicParse(httpConn_t *cp, unsigned char *buf, uint32 len,
	int32 trace);

#endif

/******************************************************************************/

#ifdef __cplusplus
}
#endif

#endif //DEMO_SSL_SERVER

#endif /* _h_MATRIXSSLAPP */

/******************************************************************************/

