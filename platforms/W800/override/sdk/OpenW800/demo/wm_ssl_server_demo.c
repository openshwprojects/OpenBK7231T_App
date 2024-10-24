#include <string.h>
#include "wm_include.h"
#include "wm_ssl_server_demo.h"
#include "wm_demo.h"

#if DEMO_SSL_SERVER

#define    DEMO_SSL_SERVER_TASK_SIZE      2000
tls_os_queue_t *demo_ssl_server_q = NULL;
static OS_STK DemoSSLServerTaskStk[DEMO_SSL_SERVER_TASK_SIZE]; 

#define BACKLOG 7 

#if TLS_CONFIG_USE_MBEDTLS

static const char demo_test_srv_crt[] =
"-----BEGIN CERTIFICATE-----\r\n"
"MIIDNzCCAh+gAwIBAgIBAjANBgkqhkiG9w0BAQUFADA7MQswCQYDVQQGEwJOTDER\r\n"
"MA8GA1UEChMIUG9sYXJTU0wxGTAXBgNVBAMTEFBvbGFyU1NMIFRlc3QgQ0EwHhcN\r\n"
"MTEwMjEyMTQ0NDA2WhcNMjEwMjEyMTQ0NDA2WjA0MQswCQYDVQQGEwJOTDERMA8G\r\n"
"A1UEChMIUG9sYXJTU0wxEjAQBgNVBAMTCWxvY2FsaG9zdDCCASIwDQYJKoZIhvcN\r\n"
"AQEBBQADggEPADCCAQoCggEBAMFNo93nzR3RBNdJcriZrA545Do8Ss86ExbQWuTN\r\n"
"owCIp+4ea5anUrSQ7y1yej4kmvy2NKwk9XfgJmSMnLAofaHa6ozmyRyWvP7BBFKz\r\n"
"NtSj+uGxdtiQwWG0ZlI2oiZTqqt0Xgd9GYLbKtgfoNkNHC1JZvdbJXNG6AuKT2kM\r\n"
"tQCQ4dqCEGZ9rlQri2V5kaHiYcPNQEkI7mgM8YuG0ka/0LiqEQMef1aoGh5EGA8P\r\n"
"hYvai0Re4hjGYi/HZo36Xdh98yeJKQHFkA4/J/EwyEoO79bex8cna8cFPXrEAjya\r\n"
"HT4P6DSYW8tzS1KW2BGiLICIaTla0w+w3lkvEcf36hIBMJcCAwEAAaNNMEswCQYD\r\n"
"VR0TBAIwADAdBgNVHQ4EFgQUpQXoZLjc32APUBJNYKhkr02LQ5MwHwYDVR0jBBgw\r\n"
"FoAUtFrkpbPe0lL2udWmlQ/rPrzH/f8wDQYJKoZIhvcNAQEFBQADggEBAJxnXClY\r\n"
"oHkbp70cqBrsGXLybA74czbO5RdLEgFs7rHVS9r+c293luS/KdliLScZqAzYVylw\r\n"
"UfRWvKMoWhHYKp3dEIS4xTXk6/5zXxhv9Rw8SGc8qn6vITHk1S1mPevtekgasY5Y\r\n"
"iWQuM3h4YVlRH3HHEMAD1TnAexfXHHDFQGe+Bd1iAbz1/sH9H8l4StwX6egvTK3M\r\n"
"wXRwkKkvjKaEDA9ATbZx0mI8LGsxSuCqe9r9dyjmttd47J1p1Rulz3CLzaRcVIuS\r\n"
"RRQfaD8neM9c1S/iJ/amTVqJxA1KOdOS5780WhPfSArA+g4qAmSjelc3p4wWpha8\r\n"
"zhuYwjVuX6JHG0c=\r\n"
"-----END CERTIFICATE-----\r\n";

static const char demo_test_cas_pem[] =
"-----BEGIN CERTIFICATE-----\r\n"                                       \
"MIIDhzCCAm+gAwIBAgIBADANBgkqhkiG9w0BAQUFADA7MQswCQYDVQQGEwJOTDER\r\n"  \
"MA8GA1UEChMIUG9sYXJTU0wxGTAXBgNVBAMTEFBvbGFyU1NMIFRlc3QgQ0EwHhcN\r\n"  \
"MTEwMjEyMTQ0NDAwWhcNMjEwMjEyMTQ0NDAwWjA7MQswCQYDVQQGEwJOTDERMA8G\r\n"  \
"A1UEChMIUG9sYXJTU0wxGTAXBgNVBAMTEFBvbGFyU1NMIFRlc3QgQ0EwggEiMA0G\r\n"  \
"CSqGSIb3DQEBAQUAA4IBDwAwggEKAoIBAQDA3zf8F7vglp0/ht6WMn1EpRagzSHx\r\n"  \
"mdTs6st8GFgIlKXsm8WL3xoemTiZhx57wI053zhdcHgH057Zk+i5clHFzqMwUqny\r\n"  \
"50BwFMtEonILwuVA+T7lpg6z+exKY8C4KQB0nFc7qKUEkHHxvYPZP9al4jwqj+8n\r\n"  \
"YMPGn8u67GB9t+aEMr5P+1gmIgNb1LTV+/Xjli5wwOQuvfwu7uJBVcA0Ln0kcmnL\r\n"  \
"R7EUQIN9Z/SG9jGr8XmksrUuEvmEF/Bibyc+E1ixVA0hmnM3oTDPb5Lc9un8rNsu\r\n"  \
"KNF+AksjoBXyOGVkCeoMbo4bF6BxyLObyavpw/LPh5aPgAIynplYb6LVAgMBAAGj\r\n"  \
"gZUwgZIwDAYDVR0TBAUwAwEB/zAdBgNVHQ4EFgQUtFrkpbPe0lL2udWmlQ/rPrzH\r\n"  \
"/f8wYwYDVR0jBFwwWoAUtFrkpbPe0lL2udWmlQ/rPrzH/f+hP6Q9MDsxCzAJBgNV\r\n"  \
"BAYTAk5MMREwDwYDVQQKEwhQb2xhclNTTDEZMBcGA1UEAxMQUG9sYXJTU0wgVGVz\r\n"  \
"dCBDQYIBADANBgkqhkiG9w0BAQUFAAOCAQEAuP1U2ABUkIslsCfdlc2i94QHHYeJ\r\n"  \
"SsR4EdgHtdciUI5I62J6Mom+Y0dT/7a+8S6MVMCZP6C5NyNyXw1GWY/YR82XTJ8H\r\n"  \
"DBJiCTok5DbZ6SzaONBzdWHXwWwmi5vg1dxn7YxrM9d0IjxM27WNKs4sDQhZBQkF\r\n"  \
"pjmfs2cb4oPl4Y9T9meTx/lvdkRYEug61Jfn6cA+qHpyPYdTH+UshITnmp5/Ztkf\r\n"  \
"m/UTSLBNFNHesiTZeH31NcxYGdHSme9Nc/gfidRa0FLOCfWxRlFqAI47zG9jAQCZ\r\n"  \
"7Z2mCGDNMhjQc+BYcdnl0lPXjdDK6V0qCg1dVewhUBcW5gZKzV7e9+DpVA==\r\n"      \
"-----END CERTIFICATE-----\r\n";

static const char demo_test_srv_key[] =
"-----BEGIN RSA PRIVATE KEY-----\r\n"
"MIIEpAIBAAKCAQEAwU2j3efNHdEE10lyuJmsDnjkOjxKzzoTFtBa5M2jAIin7h5r\r\n"
"lqdStJDvLXJ6PiSa/LY0rCT1d+AmZIycsCh9odrqjObJHJa8/sEEUrM21KP64bF2\r\n"
"2JDBYbRmUjaiJlOqq3ReB30Zgtsq2B+g2Q0cLUlm91slc0boC4pPaQy1AJDh2oIQ\r\n"
"Zn2uVCuLZXmRoeJhw81ASQjuaAzxi4bSRr/QuKoRAx5/VqgaHkQYDw+Fi9qLRF7i\r\n"
"GMZiL8dmjfpd2H3zJ4kpAcWQDj8n8TDISg7v1t7HxydrxwU9esQCPJodPg/oNJhb\r\n"
"y3NLUpbYEaIsgIhpOVrTD7DeWS8Rx/fqEgEwlwIDAQABAoIBAQCXR0S8EIHFGORZ\r\n"
"++AtOg6eENxD+xVs0f1IeGz57Tjo3QnXX7VBZNdj+p1ECvhCE/G7XnkgU5hLZX+G\r\n"
"Z0jkz/tqJOI0vRSdLBbipHnWouyBQ4e/A1yIJdlBtqXxJ1KE/ituHRbNc4j4kL8Z\r\n"
"/r6pvwnTI0PSx2Eqs048YdS92LT6qAv4flbNDxMn2uY7s4ycS4Q8w1JXnCeaAnYm\r\n"
"WYI5wxO+bvRELR2Mcz5DmVnL8jRyml6l6582bSv5oufReFIbyPZbQWlXgYnpu6He\r\n"
"GTc7E1zKYQGG/9+DQUl/1vQuCPqQwny0tQoX2w5tdYpdMdVm+zkLtbajzdTviJJa\r\n"
"TWzL6lt5AoGBAN86+SVeJDcmQJcv4Eq6UhtRr4QGMiQMz0Sod6ettYxYzMgxtw28\r\n"
"CIrgpozCc+UaZJLo7UxvC6an85r1b2nKPCLQFaggJ0H4Q0J/sZOhBIXaoBzWxveK\r\n"
"nupceKdVxGsFi8CDy86DBfiyFivfBj+47BbaQzPBj7C4rK7UlLjab2rDAoGBAN2u\r\n"
"AM2gchoFiu4v1HFL8D7lweEpi6ZnMJjnEu/dEgGQJFjwdpLnPbsj4c75odQ4Gz8g\r\n"
"sw9lao9VVzbusoRE/JGI4aTdO0pATXyG7eG1Qu+5Yc1YGXcCrliA2xM9xx+d7f+s\r\n"
"mPzN+WIEg5GJDYZDjAzHG5BNvi/FfM1C9dOtjv2dAoGAF0t5KmwbjWHBhcVqO4Ic\r\n"
"BVvN3BIlc1ue2YRXEDlxY5b0r8N4XceMgKmW18OHApZxfl8uPDauWZLXOgl4uepv\r\n"
"whZC3EuWrSyyICNhLY21Ah7hbIEBPF3L3ZsOwC+UErL+dXWLdB56Jgy3gZaBeW7b\r\n"
"vDrEnocJbqCm7IukhXHOBK8CgYEAwqdHB0hqyNSzIOGY7v9abzB6pUdA3BZiQvEs\r\n"
"3LjHVd4HPJ2x0N8CgrBIWOE0q8+0hSMmeE96WW/7jD3fPWwCR5zlXknxBQsfv0gP\r\n"
"3BC5PR0Qdypz+d+9zfMf625kyit4T/hzwhDveZUzHnk1Cf+IG7Q+TOEnLnWAWBED\r\n"
"ISOWmrUCgYAFEmRxgwAc/u+D6t0syCwAYh6POtscq9Y0i9GyWk89NzgC4NdwwbBH\r\n"
"4AgahOxIxXx2gxJnq3yfkJfIjwf0s2DyP0kY2y6Ua1OeomPeY9mrIS4tCuDQ6LrE\r\n"
"TB6l9VGoxJL4fyHnZb8L5gGvnB1bbD8cL6YPaDiOhcRseC9vBiEuVg==\r\n"
"-----END RSA PRIVATE KEY-----\r\n";

/********************************** Globals ***********************************/
#define RECV_BUF_LEN 1024
static char	g_httpResponseHdr[] = \
    "HTTP/1.0 200 OK\r\nContent-Type: text/html\r\n\r\n" \
    "<h2>mbed TLS Test Server</h2>\r\n" \
    "<p>Successful connection.</p>\r\n";
static char RECV_BUF[RECV_BUF_LEN];

/******************************************************************************/
/*
	Make sure the socket is not inherited by exec'd processes
	Set the REUSE flag to minimize the number of sockets in TIME_WAIT
	Then we set REUSEADDR, NODELAY and NONBLOCK on the socket
*/
static void setSocketOptions(SOCKET fd)
{
	int32 rc;
	rc = 1;
	setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (char *)&rc, sizeof(rc));
}

/******************************************************************************/
/*
	Establish a listening socket for incomming connections
 */
static SOCKET socketListen(short port, int32 *err)
{
	struct sockaddr_in	addr;
	SOCKET				fd;
	
	if ((fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		printf("Error creating listen socket\n");
		*err = SOCKET_ERRNO;
		return INVALID_SOCKET;
	}
	
	setSocketOptions(fd);
	
	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);
	addr.sin_addr.s_addr = INADDR_ANY;
	if (bind(fd, (struct sockaddr *) &addr, sizeof(addr)) < 0) {
		printf("Can't bind socket. Port in use or insufficient privilege\n");
		*err = SOCKET_ERRNO;
		return INVALID_SOCKET;
	}
	if (listen(fd, BACKLOG) < 0) {
		printf("Error listening on socket\n");
		*err = SOCKET_ERRNO;
		return INVALID_SOCKET;
	}
	printf("Listening on port %d\n", port);
	return fd;
}

/******************************************************************************/
/*
	Non-blocking socket event handler
	Wait one time in select for events on any socket
	This will accept new connections, read and write to sockets that are
	connected, and close sockets as required.
 */
static int32 selectLoop(tls_ssl_t *keys, SOCKET lfd)
{
	tls_ssl_t *ssl;
	SOCKET     fd;
	char      *buf;
	int32     rc, len;

    do
	{
		fd = accept(lfd, NULL, NULL);
		if (fd == INVALID_SOCKET) {
			break;	/* Nothing more to accept; next listener */
		}
		printf("accept fd %d\n", fd);
		setSocketOptions(fd);

		if ((rc = tls_ssl_server_handshake(&ssl, fd, (tls_ssl_key_t *)keys)) < 0) {
			printf("tls_ssl_server_handshake rc %d\n", rc);
			close(fd); fd = INVALID_SOCKET;
			continue;
		}

		printf("ssl handshake ok\n");

		buf = RECV_BUF;
		len = RECV_BUF_LEN - 1;
		rc = tls_ssl_server_recv(ssl, fd, buf, len, 0);
		if (rc > 0)
		{
		    buf[rc] = '\0';
            printf("recvd %d bytes: %s\n\n", rc, buf);

            rc = tls_ssl_server_send(ssl, fd, g_httpResponseHdr, strlen(g_httpResponseHdr), 0);
            printf( "%d bytes written\n\n%s\n", rc, g_httpResponseHdr );
		}
		else
		{
            printf("ssl recv err, rc = %d\n", rc);
		}

		printf( "closing the connection...\n" );

		tls_ssl_server_close_conn(ssl, fd);
		close(fd);
	} while(1);

	return 0;
}

/******************************************************************************/
/*
	non-blocking SSL server
	Initialize MatrixSSL and sockets layer, and loop on select
 */
int32 server_idle(int proto_ver)
{
	tls_ssl_key_t *keys = NULL;
	SOCKET         lfd = INVALID_SOCKET;
	int32          err, rc;

	keys = NULL;
	lfd = INVALID_SOCKET;

	if ((rc=tls_ssl_server_init((void*)proto_ver)) < 0) {
		printf("tls_ssl_server_init key init failure.  Exiting\n");
		return rc;
	}

	if (tls_ssl_server_load_keys(&keys,	
                                (unsigned char *)demo_test_srv_crt, sizeof(demo_test_srv_crt),
                                (unsigned char *)demo_test_srv_key, sizeof(demo_test_srv_key), 
                                (unsigned char *)demo_test_cas_pem, sizeof(demo_test_cas_pem), 
                                KEY_RSA) < 0) {
		printf("tls_ssl_server_load_keys key init failure.  Exiting\n");
		goto L_EXIT;
	}

	/* Create the listening socket that will accept incoming connections */
	if ((lfd = socketListen(HTTPS_PORT, &err)) == INVALID_SOCKET) {
		printf("Can't listen on port %d\n", HTTPS_PORT);
		goto L_EXIT;
	}

	/* Main select loop to handle sockets events */
	while (1) {
		selectLoop((tls_ssl_t *)keys, lfd);
	}

L_EXIT:
	if (lfd != INVALID_SOCKET) close(lfd);
	tls_ssl_server_close(keys);
	
	return 0;
}


#else

#define ALLOW_ANON_CONNECTIONS	1
#define USE_HEADER_KEYS

/* Identity Key and Cert */
unsigned char RSA1024[] = { 
48, 130, 2, 255, 48, 130, 2, 104, 160, 3, 2, 1, 2, 
2, 5, 49, 50, 51, 52, 53, 48, 13, 6, 9, 42, 134, 
72, 134, 247, 13, 1, 1, 11, 5, 0, 48, 129, 150, 49, 
53, 48, 51, 6, 3, 85, 4, 3, 12, 44, 83, 97, 109, 
112, 108, 101, 32, 77, 97, 116, 114, 105, 120, 32, 82, 83, 
65, 45, 49, 48, 50, 52, 32, 67, 101, 114, 116, 105, 102, 
105, 99, 97, 116, 101, 32, 65, 117, 116, 104, 111, 114, 105, 
116, 121, 49, 11, 48, 9, 6, 3, 85, 4, 6, 12, 2, 
85, 83, 49, 11, 48, 9, 6, 3, 85, 4, 8, 12, 2, 
87, 65, 49, 16, 48, 14, 6, 3, 85, 4, 7, 12, 7, 
83, 101, 97, 116, 116, 108, 101, 49, 34, 48, 32, 6, 3, 
85, 4, 10, 12, 25, 73, 78, 83, 73, 68, 69, 32, 83, 
101, 99, 117, 114, 101, 32, 67, 111, 114, 112, 111, 114, 97, 
116, 105, 111, 110, 49, 13, 48, 11, 6, 3, 85, 4, 11, 
12, 4, 84, 101, 115, 116, 48, 30, 23, 13, 49, 52, 48, 
51, 50, 52, 49, 54, 51, 54, 52, 51, 90, 23, 13, 49, 
55, 48, 51, 50, 51, 49, 54, 51, 54, 52, 51, 90, 48, 
129, 140, 49, 43, 48, 41, 6, 3, 85, 4, 3, 12, 34, 
83, 97, 109, 112, 108, 101, 32, 77, 97, 116, 114, 105, 120, 
32, 82, 83, 65, 45, 49, 48, 50, 52, 32, 67, 101, 114, 
116, 105, 102, 105, 99, 97, 116, 101, 49, 11, 48, 9, 6, 
3, 85, 4, 6, 12, 2, 85, 83, 49, 11, 48, 9, 6, 
3, 85, 4, 8, 12, 2, 87, 65, 49, 16, 48, 14, 6, 
3, 85, 4, 7, 12, 7, 83, 101, 97, 116, 116, 108, 101, 
49, 34, 48, 32, 6, 3, 85, 4, 10, 12, 25, 73, 78, 
83, 73, 68, 69, 32, 83, 101, 99, 117, 114, 101, 32, 67, 
111, 114, 112, 111, 114, 97, 116, 105, 111, 110, 49, 13, 48, 
11, 6, 3, 85, 4, 11, 12, 4, 84, 101, 115, 116, 48, 
129, 159, 48, 13, 6, 9, 42, 134, 72, 134, 247, 13, 1, 
1, 1, 5, 0, 3, 129, 141, 0, 48, 129, 137, 2, 129, 
129, 0, 171, 74, 251, 133, 203, 23, 206, 121, 129, 83, 106, 
128, 62, 40, 8, 135, 8, 10, 136, 83, 189, 35, 151, 34, 
17, 142, 82, 217, 252, 181, 153, 110, 93, 92, 164, 77, 244, 
112, 162, 136, 97, 197, 170, 99, 230, 154, 247, 244, 140, 4, 
166, 187, 118, 131, 170, 65, 194, 38, 148, 205, 157, 85, 0, 
127, 225, 255, 251, 189, 133, 119, 180, 105, 24, 126, 169, 72, 
163, 33, 39, 79, 122, 157, 50, 195, 182, 155, 57, 104, 184, 
118, 16, 186, 132, 134, 209, 236, 1, 204, 202, 31, 193, 74, 
90, 185, 32, 153, 141, 57, 243, 174, 93, 17, 124, 35, 39, 
82, 149, 82, 92, 137, 0, 138, 137, 234, 208, 194, 69, 127, 
2, 3, 1, 0, 1, 163, 97, 48, 95, 48, 31, 6, 3, 
85, 29, 35, 4, 24, 48, 22, 128, 20, 247, 36, 136, 131, 
147, 137, 77, 217, 3, 85, 193, 56, 39, 23, 64, 229, 236, 
225, 212, 176, 48, 26, 6, 3, 85, 29, 17, 4, 19, 48, 
17, 130, 9, 108, 111, 99, 97, 108, 104, 111, 115, 116, 135, 
4, 127, 0, 0, 1, 48, 32, 6, 3, 85, 29, 37, 1, 
1, 255, 4, 22, 48, 20, 6, 8, 43, 6, 1, 5, 5, 
7, 3, 1, 6, 8, 43, 6, 1, 5, 5, 7, 3, 2, 
48, 13, 6, 9, 42, 134, 72, 134, 247, 13, 1, 1, 11, 
5, 0, 3, 129, 129, 0, 27, 242, 239, 83, 0, 98, 175, 
14, 102, 3, 57, 93, 130, 103, 97, 81, 59, 81, 193, 229, 
138, 142, 145, 142, 16, 166, 84, 91, 38, 222, 2, 25, 214, 
176, 5, 228, 167, 122, 82, 176, 116, 161, 45, 87, 78, 22, 
185, 255, 158, 42, 206, 84, 191, 197, 250, 159, 157, 143, 76, 
175, 27, 54, 163, 2, 88, 216, 148, 232, 206, 188, 75, 131, 
230, 119, 52, 136, 70, 233, 233, 138, 146, 167, 220, 140, 47, 
103, 11, 213, 174, 147, 37, 83, 99, 7, 33, 240, 74, 35, 
150, 133, 20, 147, 3, 166, 156, 96, 232, 88, 39, 97, 187, 
185, 107, 45, 144, 211, 153, 25, 9, 219, 180, 205, 216, 68, 
137, 34, 222, 103};
unsigned char RSA1024KEY[] = { 
48, 130, 2, 93, 2, 1, 0, 2, 129, 129, 0, 171, 74, 
251, 133, 203, 23, 206, 121, 129, 83, 106, 128, 62, 40, 8, 
135, 8, 10, 136, 83, 189, 35, 151, 34, 17, 142, 82, 217, 
252, 181, 153, 110, 93, 92, 164, 77, 244, 112, 162, 136, 97, 
197, 170, 99, 230, 154, 247, 244, 140, 4, 166, 187, 118, 131, 
170, 65, 194, 38, 148, 205, 157, 85, 0, 127, 225, 255, 251, 
189, 133, 119, 180, 105, 24, 126, 169, 72, 163, 33, 39, 79, 
122, 157, 50, 195, 182, 155, 57, 104, 184, 118, 16, 186, 132, 
134, 209, 236, 1, 204, 202, 31, 193, 74, 90, 185, 32, 153, 
141, 57, 243, 174, 93, 17, 124, 35, 39, 82, 149, 82, 92, 
137, 0, 138, 137, 234, 208, 194, 69, 127, 2, 3, 1, 0, 
1, 2, 129, 128, 43, 29, 132, 145, 248, 188, 213, 75, 224, 
49, 142, 237, 24, 184, 26, 237, 98, 40, 196, 135, 207, 99, 
213, 246, 205, 84, 117, 166, 72, 229, 172, 233, 10, 182, 246, 
228, 104, 82, 177, 150, 130, 133, 174, 141, 214, 205, 202, 251, 
211, 2, 152, 181, 54, 239, 141, 59, 96, 19, 54, 1, 75, 
72, 202, 6, 252, 191, 158, 253, 31, 91, 148, 174, 143, 126, 
211, 216, 136, 54, 13, 80, 143, 101, 121, 179, 170, 105, 222, 
100, 47, 185, 67, 68, 50, 254, 228, 43, 126, 74, 126, 221, 
184, 247, 27, 116, 140, 46, 214, 115, 10, 14, 220, 251, 133, 
131, 109, 59, 161, 234, 198, 91, 90, 225, 44, 228, 71, 245, 
81, 129, 2, 65, 0, 224, 116, 248, 238, 225, 240, 181, 182, 
188, 244, 4, 121, 44, 29, 207, 125, 174, 145, 161, 47, 130, 
148, 232, 80, 127, 111, 136, 172, 54, 189, 120, 230, 60, 254, 
134, 231, 137, 101, 130, 4, 73, 145, 24, 152, 162, 1, 69, 
185, 7, 124, 205, 44, 133, 203, 123, 22, 142, 211, 42, 110, 
241, 171, 126, 195, 2, 65, 0, 195, 93, 99, 178, 28, 17, 
60, 157, 111, 114, 66, 30, 188, 16, 43, 69, 79, 14, 40, 
167, 66, 55, 88, 19, 200, 85, 241, 32, 148, 192, 203, 84, 
49, 183, 182, 14, 97, 200, 57, 29, 129, 80, 235, 74, 40, 
46, 25, 107, 73, 181, 180, 76, 16, 107, 175, 240, 144, 16, 
37, 12, 78, 236, 170, 149, 2, 64, 90, 233, 230, 30, 83, 
243, 188, 150, 108, 200, 101, 187, 114, 204, 12, 75, 250, 214, 
79, 180, 63, 174, 242, 190, 16, 47, 235, 234, 221, 45, 55, 
120, 2, 66, 145, 255, 220, 93, 250, 32, 164, 111, 153, 199, 
208, 238, 180, 255, 241, 241, 113, 229, 83, 184, 12, 126, 33, 
145, 148, 129, 101, 111, 178, 35, 2, 65, 0, 178, 135, 251, 
125, 94, 108, 218, 9, 189, 56, 154, 247, 223, 64, 159, 106, 
93, 14, 65, 84, 104, 12, 143, 110, 180, 154, 229, 25, 179, 
22, 100, 57, 114, 157, 193, 191, 110, 216, 60, 105, 156, 45, 
72, 119, 162, 52, 78, 130, 94, 255, 175, 221, 250, 251, 141, 
242, 182, 148, 42, 103, 15, 16, 243, 233, 2, 65, 0, 222, 
122, 102, 45, 182, 99, 164, 2, 140, 85, 237, 44, 125, 145, 
115, 171, 101, 118, 197, 12, 133, 100, 128, 181, 144, 193, 72, 
128, 21, 13, 96, 223, 111, 23, 146, 63, 219, 25, 209, 127, 
240, 137, 63, 109, 167, 41, 217, 24, 233, 148, 219, 125, 106, 
104, 231, 188, 243, 12, 248, 24, 47, 184, 221, 228};


/*	CA files for client auth are selected more generously.  If the algorithm
	type is supported, we'll load it */
unsigned char RSACAS[] = { 
48, 130, 3, 13, 48, 130, 2, 118, 160, 3, 2, 1, 2, 
2, 4, 49, 50, 51, 52, 48, 13, 6, 9, 42, 134, 72, 
134, 247, 13, 1, 1, 11, 5, 0, 48, 129, 150, 49, 53, 
48, 51, 6, 3, 85, 4, 3, 12, 44, 83, 97, 109, 112, 
108, 101, 32, 77, 97, 116, 114, 105, 120, 32, 82, 83, 65, 
45, 49, 48, 50, 52, 32, 67, 101, 114, 116, 105, 102, 105, 
99, 97, 116, 101, 32, 65, 117, 116, 104, 111, 114, 105, 116, 
121, 49, 11, 48, 9, 6, 3, 85, 4, 6, 12, 2, 85, 
83, 49, 11, 48, 9, 6, 3, 85, 4, 8, 12, 2, 87, 
65, 49, 16, 48, 14, 6, 3, 85, 4, 7, 12, 7, 83, 
101, 97, 116, 116, 108, 101, 49, 34, 48, 32, 6, 3, 85, 
4, 10, 12, 25, 73, 78, 83, 73, 68, 69, 32, 83, 101, 
99, 117, 114, 101, 32, 67, 111, 114, 112, 111, 114, 97, 116, 
105, 111, 110, 49, 13, 48, 11, 6, 3, 85, 4, 11, 12, 
4, 84, 101, 115, 116, 48, 30, 23, 13, 49, 52, 48, 51, 
50, 52, 49, 54, 50, 54, 52, 54, 90, 23, 13, 49, 55, 
48, 51, 50, 51, 49, 54, 50, 54, 52, 54, 90, 48, 129, 
150, 49, 53, 48, 51, 6, 3, 85, 4, 3, 12, 44, 83, 
97, 109, 112, 108, 101, 32, 77, 97, 116, 114, 105, 120, 32, 
82, 83, 65, 45, 49, 48, 50, 52, 32, 67, 101, 114, 116, 
105, 102, 105, 99, 97, 116, 101, 32, 65, 117, 116, 104, 111, 
114, 105, 116, 121, 49, 11, 48, 9, 6, 3, 85, 4, 6, 
12, 2, 85, 83, 49, 11, 48, 9, 6, 3, 85, 4, 8, 
12, 2, 87, 65, 49, 16, 48, 14, 6, 3, 85, 4, 7, 
12, 7, 83, 101, 97, 116, 116, 108, 101, 49, 34, 48, 32, 
6, 3, 85, 4, 10, 12, 25, 73, 78, 83, 73, 68, 69, 
32, 83, 101, 99, 117, 114, 101, 32, 67, 111, 114, 112, 111, 
114, 97, 116, 105, 111, 110, 49, 13, 48, 11, 6, 3, 85, 
4, 11, 12, 4, 84, 101, 115, 116, 48, 129, 159, 48, 13, 
6, 9, 42, 134, 72, 134, 247, 13, 1, 1, 1, 5, 0, 
3, 129, 141, 0, 48, 129, 137, 2, 129, 129, 0, 191, 64, 
80, 242, 226, 56, 57, 33, 56, 116, 145, 34, 113, 4, 29, 
198, 49, 53, 74, 169, 55, 198, 177, 97, 20, 225, 167, 222, 
111, 25, 15, 207, 20, 160, 234, 84, 115, 214, 32, 157, 55, 
52, 128, 187, 198, 116, 140, 77, 156, 81, 22, 13, 214, 52, 
231, 167, 4, 188, 224, 147, 232, 31, 154, 62, 152, 220, 93, 
22, 227, 213, 225, 134, 34, 223, 98, 137, 155, 103, 206, 132, 
218, 48, 118, 168, 205, 32, 199, 27, 53, 112, 168, 226, 170, 
45, 218, 168, 140, 48, 181, 44, 34, 12, 229, 83, 17, 180, 
181, 22, 13, 28, 185, 159, 245, 224, 66, 193, 232, 3, 210, 
182, 123, 113, 46, 167, 1, 138, 105, 249, 2, 3, 1, 0, 
1, 163, 102, 48, 100, 48, 18, 6, 3, 85, 29, 19, 1, 
1, 255, 4, 8, 48, 6, 1, 1, 255, 2, 1, 0, 48, 
29, 6, 3, 85, 29, 14, 4, 22, 4, 20, 247, 36, 136, 
131, 147, 137, 77, 217, 3, 85, 193, 56, 39, 23, 64, 229, 
236, 225, 212, 176, 48, 31, 6, 3, 85, 29, 35, 4, 24, 
48, 22, 128, 20, 247, 36, 136, 131, 147, 137, 77, 217, 3, 
85, 193, 56, 39, 23, 64, 229, 236, 225, 212, 176, 48, 14, 
6, 3, 85, 29, 15, 1, 1, 255, 4, 4, 3, 2, 0, 
4, 48, 13, 6, 9, 42, 134, 72, 134, 247, 13, 1, 1, 
11, 5, 0, 3, 129, 129, 0, 65, 150, 217, 193, 56, 223, 
116, 222, 228, 127, 198, 122, 215, 202, 221, 239, 249, 204, 117, 
121, 131, 172, 55, 115, 221, 100, 155, 10, 152, 132, 127, 157, 
102, 66, 199, 205, 140, 152, 28, 245, 122, 49, 213, 23, 103, 
168, 201, 98, 142, 162, 54, 138, 252, 218, 248, 100, 205, 156, 
107, 120, 45, 239, 124, 243, 202, 202, 227, 134, 199, 133, 247, 
147, 39, 81, 67, 84, 247, 188, 185, 208, 39, 24, 109, 198, 
186, 14, 224, 197, 23, 172, 85, 35, 162, 180, 31, 28, 86, 
12, 39, 129, 68, 66, 64, 207, 16, 27, 34, 12, 211, 137, 
159, 9, 242, 243, 51, 107, 211, 28, 59, 146, 167, 171, 94, 
189, 58, 233, 130, 140,
48, 130, 4, 20, 48, 130, 2, 252, 160, 3, 2, 1, 2, 
2, 6, 50, 51, 52, 53, 54, 55, 48, 13, 6, 9, 42, 
134, 72, 134, 247, 13, 1, 1, 11, 5, 0, 48, 129, 150, 
49, 53, 48, 51, 6, 3, 85, 4, 3, 12, 44, 83, 97, 
109, 112, 108, 101, 32, 77, 97, 116, 114, 105, 120, 32, 82, 
83, 65, 45, 50, 48, 52, 56, 32, 67, 101, 114, 116, 105, 
102, 105, 99, 97, 116, 101, 32, 65, 117, 116, 104, 111, 114, 
105, 116, 121, 49, 11, 48, 9, 6, 3, 85, 4, 6, 12, 
2, 85, 83, 49, 11, 48, 9, 6, 3, 85, 4, 8, 12, 
2, 87, 65, 49, 16, 48, 14, 6, 3, 85, 4, 7, 12, 
7, 83, 101, 97, 116, 116, 108, 101, 49, 34, 48, 32, 6, 
3, 85, 4, 10, 12, 25, 73, 78, 83, 73, 68, 69, 32, 
83, 101, 99, 117, 114, 101, 32, 67, 111, 114, 112, 111, 114, 
97, 116, 105, 111, 110, 49, 13, 48, 11, 6, 3, 85, 4, 
11, 12, 4, 84, 101, 115, 116, 48, 30, 23, 13, 49, 52, 
48, 51, 50, 52, 49, 54, 50, 55, 48, 51, 90, 23, 13, 
49, 55, 48, 51, 50, 51, 49, 54, 50, 55, 48, 51, 90, 
48, 129, 150, 49, 53, 48, 51, 6, 3, 85, 4, 3, 12, 
44, 83, 97, 109, 112, 108, 101, 32, 77, 97, 116, 114, 105, 
120, 32, 82, 83, 65, 45, 50, 48, 52, 56, 32, 67, 101, 
114, 116, 105, 102, 105, 99, 97, 116, 101, 32, 65, 117, 116, 
104, 111, 114, 105, 116, 121, 49, 11, 48, 9, 6, 3, 85, 
4, 6, 12, 2, 85, 83, 49, 11, 48, 9, 6, 3, 85, 
4, 8, 12, 2, 87, 65, 49, 16, 48, 14, 6, 3, 85, 
4, 7, 12, 7, 83, 101, 97, 116, 116, 108, 101, 49, 34, 
48, 32, 6, 3, 85, 4, 10, 12, 25, 73, 78, 83, 73, 
68, 69, 32, 83, 101, 99, 117, 114, 101, 32, 67, 111, 114, 
112, 111, 114, 97, 116, 105, 111, 110, 49, 13, 48, 11, 6, 
3, 85, 4, 11, 12, 4, 84, 101, 115, 116, 48, 130, 1, 
34, 48, 13, 6, 9, 42, 134, 72, 134, 247, 13, 1, 1, 
1, 5, 0, 3, 130, 1, 15, 0, 48, 130, 1, 10, 2, 
130, 1, 1, 0, 204, 171, 91, 64, 59, 10, 75, 192, 131, 
92, 104, 229, 244, 0, 90, 21, 26, 227, 120, 243, 160, 65, 
35, 147, 90, 193, 198, 250, 4, 144, 163, 69, 142, 48, 92, 
145, 138, 109, 132, 191, 120, 192, 203, 177, 238, 101, 175, 170, 
93, 186, 241, 53, 4, 181, 64, 148, 237, 188, 140, 245, 235, 
161, 187, 143, 162, 250, 183, 170, 236, 83, 119, 139, 175, 182, 
209, 120, 241, 88, 89, 0, 108, 143, 152, 68, 29, 115, 76, 
225, 26, 138, 176, 97, 128, 221, 192, 223, 155, 116, 95, 208, 
124, 102, 102, 35, 220, 48, 156, 77, 224, 236, 218, 110, 4, 
196, 200, 98, 162, 33, 17, 239, 40, 220, 109, 233, 49, 183, 
122, 158, 202, 219, 186, 194, 156, 147, 30, 36, 169, 154, 116, 
190, 164, 38, 78, 187, 95, 121, 4, 211, 65, 114, 5, 162, 
83, 154, 68, 32, 1, 72, 68, 93, 51, 114, 99, 63, 170, 
162, 119, 9, 245, 195, 226, 38, 181, 18, 135, 33, 173, 74, 
231, 153, 153, 239, 61, 178, 250, 189, 178, 194, 106, 206, 166, 
58, 133, 122, 63, 32, 148, 186, 21, 127, 122, 130, 233, 111, 
190, 160, 186, 234, 208, 228, 58, 57, 168, 187, 85, 96, 184, 
40, 58, 209, 163, 7, 80, 81, 245, 128, 43, 244, 90, 130, 
27, 125, 86, 169, 49, 4, 35, 177, 52, 210, 113, 213, 28, 
146, 130, 213, 182, 252, 116, 134, 6, 106, 241, 232, 114, 209, 
2, 3, 1, 0, 1, 163, 102, 48, 100, 48, 18, 6, 3, 
85, 29, 19, 1, 1, 255, 4, 8, 48, 6, 1, 1, 255, 
2, 1, 0, 48, 29, 6, 3, 85, 29, 14, 4, 22, 4, 
20, 244, 159, 233, 145, 67, 172, 28, 155, 221, 7, 64, 45, 
105, 103, 60, 239, 212, 234, 219, 84, 48, 31, 6, 3, 85, 
29, 35, 4, 24, 48, 22, 128, 20, 244, 159, 233, 145, 67, 
172, 28, 155, 221, 7, 64, 45, 105, 103, 60, 239, 212, 234, 
219, 84, 48, 14, 6, 3, 85, 29, 15, 1, 1, 255, 4, 
4, 3, 2, 0, 4, 48, 13, 6, 9, 42, 134, 72, 134, 
247, 13, 1, 1, 11, 5, 0, 3, 130, 1, 1, 0, 128, 
97, 82, 97, 34, 77, 252, 71, 207, 86, 8, 205, 176, 19, 
181, 173, 59, 182, 10, 113, 87, 194, 192, 254, 1, 255, 137, 
96, 24, 104, 238, 224, 129, 30, 156, 67, 75, 49, 166, 91, 
200, 37, 132, 75, 17, 42, 94, 250, 167, 191, 103, 142, 58, 
207, 143, 110, 234, 58, 239, 218, 196, 50, 97, 169, 93, 43, 
139, 244, 139, 43, 138, 209, 157, 24, 215, 15, 210, 155, 225, 
250, 0, 175, 214, 254, 255, 71, 29, 122, 85, 233, 131, 62, 
202, 239, 75, 13, 232, 44, 66, 246, 191, 109, 129, 10, 82, 
175, 169, 227, 249, 25, 201, 86, 64, 93, 88, 30, 241, 254, 
55, 142, 80, 23, 250, 15, 102, 156, 145, 142, 136, 231, 92, 
201, 48, 140, 53, 22, 146, 112, 122, 52, 170, 196, 240, 186, 
144, 21, 71, 44, 175, 174, 134, 12, 181, 149, 229, 40, 98, 
163, 32, 86, 191, 183, 26, 75, 79, 233, 243, 196, 24, 240, 
191, 195, 211, 122, 207, 233, 64, 159, 136, 136, 96, 117, 107, 
152, 181, 61, 171, 48, 202, 91, 149, 199, 162, 248, 102, 71, 
3, 113, 137, 192, 87, 242, 235, 179, 169, 73, 242, 16, 254, 
168, 125, 107, 144, 102, 248, 159, 70, 159, 180, 44, 95, 227, 
21, 247, 211, 68, 215, 233, 23, 61, 81, 25, 140, 46, 77, 
177, 203, 55, 112, 148, 86, 40, 140, 197, 108, 147, 19, 179, 
235, 26, 12, 112, 78, 212, 53, 47,
48, 130, 6, 19, 48, 130, 3, 251, 160, 3, 2, 1, 2, 
2, 5, 51, 52, 53, 54, 55, 48, 13, 6, 9, 42, 134, 
72, 134, 247, 13, 1, 1, 11, 5, 0, 48, 129, 150, 49, 
53, 48, 51, 6, 3, 85, 4, 3, 12, 44, 83, 97, 109, 
112, 108, 101, 32, 77, 97, 116, 114, 105, 120, 32, 82, 83, 
65, 45, 52, 48, 57, 54, 32, 67, 101, 114, 116, 105, 102, 
105, 99, 97, 116, 101, 32, 65, 117, 116, 104, 111, 114, 105, 
116, 121, 49, 11, 48, 9, 6, 3, 85, 4, 6, 12, 2, 
85, 83, 49, 11, 48, 9, 6, 3, 85, 4, 8, 12, 2, 
87, 65, 49, 16, 48, 14, 6, 3, 85, 4, 7, 12, 7, 
83, 101, 97, 116, 116, 108, 101, 49, 34, 48, 32, 6, 3, 
85, 4, 10, 12, 25, 73, 78, 83, 73, 68, 69, 32, 83, 
101, 99, 117, 114, 101, 32, 67, 111, 114, 112, 111, 114, 97, 
116, 105, 111, 110, 49, 13, 48, 11, 6, 3, 85, 4, 11, 
12, 4, 84, 101, 115, 116, 48, 30, 23, 13, 49, 52, 48, 
51, 50, 52, 49, 54, 52, 49, 48, 51, 90, 23, 13, 49, 
55, 48, 51, 50, 51, 49, 54, 52, 49, 48, 51, 90, 48, 
129, 150, 49, 53, 48, 51, 6, 3, 85, 4, 3, 12, 44, 
83, 97, 109, 112, 108, 101, 32, 77, 97, 116, 114, 105, 120, 
32, 82, 83, 65, 45, 52, 48, 57, 54, 32, 67, 101, 114, 
116, 105, 102, 105, 99, 97, 116, 101, 32, 65, 117, 116, 104, 
111, 114, 105, 116, 121, 49, 11, 48, 9, 6, 3, 85, 4, 
6, 12, 2, 85, 83, 49, 11, 48, 9, 6, 3, 85, 4, 
8, 12, 2, 87, 65, 49, 16, 48, 14, 6, 3, 85, 4, 
7, 12, 7, 83, 101, 97, 116, 116, 108, 101, 49, 34, 48, 
32, 6, 3, 85, 4, 10, 12, 25, 73, 78, 83, 73, 68, 
69, 32, 83, 101, 99, 117, 114, 101, 32, 67, 111, 114, 112, 
111, 114, 97, 116, 105, 111, 110, 49, 13, 48, 11, 6, 3, 
85, 4, 11, 12, 4, 84, 101, 115, 116, 48, 130, 2, 34, 
48, 13, 6, 9, 42, 134, 72, 134, 247, 13, 1, 1, 1, 
5, 0, 3, 130, 2, 15, 0, 48, 130, 2, 10, 2, 130, 
2, 1, 0, 187, 177, 68, 68, 161, 213, 217, 13, 226, 54, 
73, 38, 190, 105, 203, 14, 136, 74, 40, 239, 174, 44, 79, 
157, 37, 197, 138, 191, 158, 81, 228, 119, 86, 75, 77, 117, 
145, 29, 74, 210, 204, 179, 11, 47, 228, 66, 114, 250, 178, 
159, 210, 39, 224, 12, 195, 254, 131, 58, 150, 160, 212, 165, 
154, 148, 96, 186, 12, 191, 171, 142, 104, 207, 31, 196, 114, 
109, 87, 206, 96, 35, 76, 9, 244, 248, 107, 198, 89, 218, 
215, 204, 58, 194, 152, 131, 181, 252, 55, 67, 143, 28, 0, 
74, 142, 13, 73, 245, 241, 235, 166, 79, 197, 49, 13, 111, 
46, 244, 102, 10, 245, 220, 235, 175, 28, 113, 99, 225, 86, 
19, 174, 90, 202, 252, 152, 177, 99, 16, 66, 240, 155, 139, 
144, 227, 211, 215, 225, 185, 95, 15, 203, 253, 199, 116, 107, 
84, 95, 45, 209, 113, 186, 12, 148, 116, 51, 192, 62, 225, 
151, 103, 15, 42, 93, 238, 78, 220, 123, 254, 221, 43, 100, 
101, 65, 25, 176, 109, 148, 85, 24, 12, 111, 112, 60, 194, 
36, 87, 161, 252, 190, 235, 240, 240, 249, 133, 162, 163, 102, 
130, 39, 251, 112, 195, 220, 114, 95, 231, 96, 215, 59, 12, 
133, 107, 193, 197, 88, 87, 187, 173, 240, 77, 1, 163, 202, 
145, 156, 75, 151, 21, 102, 227, 25, 42, 88, 131, 68, 226, 
24, 25, 252, 144, 73, 113, 176, 89, 117, 58, 230, 116, 249, 
108, 67, 254, 2, 201, 192, 185, 168, 9, 128, 57, 60, 234, 
134, 125, 98, 75, 148, 67, 242, 74, 99, 52, 68, 3, 34, 
53, 152, 182, 155, 103, 198, 147, 100, 128, 148, 168, 153, 222, 
215, 135, 3, 26, 152, 28, 249, 208, 111, 121, 170, 222, 140, 
47, 181, 34, 169, 255, 17, 252, 170, 91, 47, 76, 197, 230, 
68, 101, 133, 228, 186, 119, 189, 51, 131, 104, 51, 86, 209, 
87, 64, 80, 0, 136, 105, 106, 22, 137, 79, 239, 144, 103, 
86, 97, 121, 105, 162, 132, 159, 237, 145, 203, 76, 187, 175, 
242, 157, 239, 227, 50, 108, 55, 100, 234, 117, 255, 197, 236, 
136, 189, 107, 67, 254, 104, 20, 212, 199, 155, 244, 174, 116, 
234, 225, 180, 99, 42, 14, 245, 156, 190, 159, 110, 78, 143, 
9, 62, 67, 141, 79, 179, 5, 37, 254, 250, 123, 230, 29, 
230, 232, 32, 27, 76, 48, 61, 224, 183, 118, 105, 11, 241, 
7, 3, 31, 143, 87, 191, 85, 207, 17, 152, 182, 153, 223, 
123, 155, 190, 152, 22, 187, 210, 129, 193, 235, 159, 8, 174, 
126, 68, 169, 167, 121, 73, 2, 172, 138, 129, 188, 146, 198, 
209, 239, 204, 157, 20, 178, 244, 9, 175, 28, 178, 11, 2, 
50, 221, 205, 252, 9, 131, 62, 28, 51, 88, 43, 119, 213, 
72, 166, 91, 117, 159, 102, 49, 17, 83, 71, 200, 199, 68, 
9, 201, 147, 196, 28, 50, 8, 105, 2, 3, 1, 0, 1, 
163, 102, 48, 100, 48, 18, 6, 3, 85, 29, 19, 1, 1, 
255, 4, 8, 48, 6, 1, 1, 255, 2, 1, 0, 48, 29, 
6, 3, 85, 29, 14, 4, 22, 4, 20, 108, 66, 132, 197, 
224, 75, 220, 112, 242, 153, 208, 28, 131, 74, 226, 81, 71, 
101, 30, 250, 48, 31, 6, 3, 85, 29, 35, 4, 24, 48, 
22, 128, 20, 108, 66, 132, 197, 224, 75, 220, 112, 242, 153, 
208, 28, 131, 74, 226, 81, 71, 101, 30, 250, 48, 14, 6, 
3, 85, 29, 15, 1, 1, 255, 4, 4, 3, 2, 0, 4, 
48, 13, 6, 9, 42, 134, 72, 134, 247, 13, 1, 1, 11, 
5, 0, 3, 130, 2, 1, 0, 102, 180, 88, 152, 103, 27, 
219, 206, 99, 95, 195, 113, 137, 160, 173, 153, 16, 192, 39, 
188, 91, 195, 160, 177, 205, 182, 237, 220, 22, 6, 51, 58, 
82, 98, 85, 7, 124, 108, 190, 199, 236, 22, 107, 127, 38, 
124, 117, 112, 41, 25, 21, 136, 64, 93, 37, 250, 80, 57, 
12, 106, 60, 199, 16, 202, 169, 185, 68, 48, 102, 147, 73, 
15, 10, 26, 111, 120, 161, 67, 179, 72, 77, 106, 145, 62, 
169, 190, 188, 217, 148, 120, 41, 224, 196, 135, 151, 148, 153, 
4, 148, 42, 33, 240, 162, 49, 152, 253, 35, 143, 192, 50, 
226, 177, 246, 228, 134, 95, 61, 148, 111, 132, 151, 62, 206, 
159, 183, 125, 134, 254, 168, 40, 44, 217, 184, 208, 239, 154, 
127, 202, 60, 218, 175, 7, 15, 223, 211, 24, 93, 39, 214, 
158, 130, 27, 9, 212, 81, 58, 117, 126, 55, 182, 226, 92, 
230, 68, 62, 81, 7, 37, 178, 230, 246, 14, 152, 68, 35, 
13, 95, 21, 236, 63, 97, 86, 95, 65, 132, 70, 59, 78, 
106, 63, 77, 108, 154, 176, 133, 133, 185, 41, 243, 179, 2, 
148, 181, 195, 150, 120, 192, 213, 97, 18, 246, 97, 10, 139, 
155, 128, 214, 60, 164, 26, 13, 77, 121, 115, 118, 164, 143, 
154, 233, 118, 243, 139, 118, 185, 109, 162, 245, 39, 187, 198, 
172, 228, 37, 1, 186, 3, 53, 53, 2, 200, 207, 173, 121, 
28, 31, 148, 62, 178, 219, 184, 167, 232, 110, 139, 68, 21, 
162, 175, 74, 103, 155, 186, 104, 73, 133, 238, 147, 196, 95, 
151, 119, 224, 180, 144, 142, 89, 236, 38, 8, 164, 82, 180, 
157, 21, 123, 92, 63, 94, 91, 91, 130, 105, 255, 249, 141, 
254, 14, 13, 34, 20, 221, 57, 106, 162, 3, 93, 223, 254, 
96, 28, 146, 249, 31, 9, 173, 173, 149, 51, 180, 176, 124, 
76, 99, 159, 48, 212, 134, 125, 138, 75, 6, 249, 106, 102, 
205, 134, 14, 252, 252, 194, 101, 169, 97, 62, 237, 172, 213, 
244, 197, 149, 143, 100, 26, 158, 20, 111, 224, 152, 184, 150, 
204, 14, 162, 177, 26, 32, 7, 96, 206, 210, 156, 48, 48, 
224, 83, 252, 31, 239, 225, 35, 171, 61, 119, 200, 208, 113, 
221, 172, 158, 28, 164, 126, 55, 118, 121, 253, 144, 91, 194, 
119, 129, 207, 65, 31, 16, 156, 23, 92, 186, 186, 12, 202, 
240, 221, 26, 187, 42, 93, 168, 198, 60, 40, 19, 221, 163, 
213, 163, 204, 52, 44, 176, 9, 66, 163, 241, 149, 215, 74, 
213, 178, 175, 163, 173, 194, 114, 103, 72, 109, 93, 90, 35, 
127, 182, 57, 247, 165, 205, 195, 1, 155, 126, 145, 87, 173, 
105, 250, 106, 27, 93, 37, 103, 200, 30, 76, 8, 108, 27, 
229, 214, 55, 160, 201, 152, 195, 102, 100, 102, 213, 114, 48, 
92, 179, 21, 200, 102, 34, 5, 46, 218, 97, 169, 202};

/********************************** Defines ***********************************/

#define SSL_TIMEOUT			45000
#define SELECT_TIME			1000

/********************************** Globals ***********************************/
#define MATRIXSSL_VERSION      "3.6.1-OPEN"
#define RECV_BUF_LEN 1024
static DLListEntry		g_conns;
static int32			g_exitFlag;
static unsigned char	g_httpResponseHdr[] = "HTTP/1.0 200 OK\r\n" 
	"Server: MatrixSSL/" MATRIXSSL_VERSION "\r\n"
	"Pragma: no-cache\r\n"
	"Cache-Control: no-cache\r\n"
	"Content-type: text/plain\r\n"
	"Content-length: 9\r\n"
	"\r\n"
	"MatrixSSL";
static char RECV_BUF[RECV_BUF_LEN];

/****************************** Local Functions *******************************/

static int32 selectLoop(tls_ssl_t *keys, SOCKET lfd);
static int32 httpWriteResponse(httpConn_t *conn);
static int32 httpBasicParse(httpConn_t *cp, unsigned char *buf, uint32 len, 
		int32 trace);
static void setSocketOptions(SOCKET fd);
static SOCKET socketListen(short port, int32 *err);
static void closeConn(httpConn_t *cp, int32 reason);


/******************************************************************************/
/*
	Non-blocking socket event handler
	Wait one time in select for events on any socket
	This will accept new connections, read and write to sockets that are
	connected, and close sockets as required.
 */
static int32 selectLoop(tls_ssl_t *keys, SOCKET lfd)
{
	httpConn_t		*cp;
	DLListEntry		connsTmp;
	DLListEntry		*pList;
	
	fd_set			readfd, writefd;
	struct timeval	timeout;
	SOCKET			fd, maxfd;
	
	char	*buf;
	int32			rc, len, val;
	u8              bReadMore = 0;

	//printf("selectLoop enter\n");
	
	DLListInit(&connsTmp);
	rc = PS_SUCCESS;
	maxfd = INVALID_SOCKET;
	timeout.tv_sec = SELECT_TIME / 1000;
	timeout.tv_usec = (SELECT_TIME % 1000) * 1000;
	FD_ZERO(&readfd);
	FD_ZERO(&writefd);
	
	/* Always set readfd for listening socket */
	FD_SET(lfd, &readfd);
	if (lfd > maxfd) {
		maxfd = lfd;
	}
/*	
	Check timeouts and set readfd and writefd for connections as required.
	We use connsTemp so that removal on error from the active iteration list
		doesn't interfere with list traversal 
 */
	while (!DLListIsEmpty(&g_conns)) {
		pList = DLListGetHead(&g_conns);
		cp = DLListGetContainer(pList, httpConn_t, List);
		DLListInsertTail(&connsTmp, &cp->List);
		
		/* Always select for read */
		FD_SET(cp->fd, &readfd);
		/* Housekeeping for maxsock in select call */
		if (cp->fd > maxfd) {
			maxfd = cp->fd;
		}
	}
	//printf("select start maxfd %d\n", maxfd);
	/* Use select to check for events on the sockets */
	if ((val = select(maxfd + 1, &readfd, &writefd, NULL, &timeout)) <= 0) {
		/* On error, restore global connections list */
		while (!DLListIsEmpty(&connsTmp)) {
			pList = DLListGetHead(&connsTmp);
			cp = DLListGetContainer(pList, httpConn_t, List);
			DLListInsertTail(&g_conns, &cp->List);
		}
		/* Select timeout */
		if (val == 0) {
			return PS_TIMEOUT_FAIL;
		}
		/* Woke due to interrupt */
		if (SOCKET_ERRNO == EINTR) {
			return PS_TIMEOUT_FAIL;
		}
		/* Should attempt to handle more errnos, such as EBADF */
		return PS_PLATFORM_FAIL;
	}
	//printf("select ret %d\n", val);
	/* Check listener for new incoming socket connections */
	if (FD_ISSET(lfd, &readfd)) {
		do
		{
			fd = accept(lfd, NULL, NULL);
			if (fd == INVALID_SOCKET) {
				break;	/* Nothing more to accept; next listener */
			}
			printf("accept fd %d\n", fd);
			setSocketOptions(fd);
			cp = tls_mem_alloc(sizeof(httpConn_t));
			printf("tls_mem_alloc cp %x\n", (u32)cp);
			memset(cp, 0x0, sizeof(httpConn_t));
			if ((rc = tls_ssl_server_handshake(&cp->ssl, fd, (tls_ssl_key_t *)keys)) < 0) {
				printf("tls_ssl_server_handshake rc %d\n", rc);
				close(fd); fd = INVALID_SOCKET;
				continue;
			}
			printf("tls_ssl_server_handshake rc %d\n", rc);
			cp->fd = fd;
			fd = INVALID_SOCKET;
			printf("cp->time.tv_sec %ld\n", cp->time.tv_sec);
			cp->parsebuf = NULL;
			cp->parsebuflen = 0;
			DLListInsertTail(&connsTmp, &cp->List);
/*			printf("=== New Client %d ===\n", cp->fd); */
		} 
		while(0);
	}
	
	/* Check each connection for read/write activity */
	while (!DLListIsEmpty(&connsTmp)) {
		pList = DLListGetHead(&connsTmp);
		cp = DLListGetContainer(pList, httpConn_t, List);
		DLListInsertTail(&g_conns, &cp->List);
		/*
		Check the file descriptor returned from select to see if the connection
		has data to be read
 */
		if (FD_ISSET(cp->fd, &readfd)) {
			printf("fd %d is set\n", cp->fd);
READ_MORE:
			buf = RECV_BUF;
			len = RECV_BUF_LEN;
			bReadMore = 0;
			rc = tls_ssl_server_recv(cp->ssl, cp->fd, buf, len, 0);
			if(SOCKET_ERROR == rc)
			{
				closeConn(cp, PS_ARG_FAIL);
				continue;	/* Next connection */
			}
			else if(rc > 0)
			{
				len = rc;
			}
			else if(SOCKET_SSL_MORE_DATA == rc)
			{
				bReadMore = 1;
			}
			printf("tls_ssl_server_recv rc %d\n", rc);
			printf("buf: %s\n", buf);
			if ((rc = httpBasicParse(cp, (u8*)buf, len, 0)) < 0) {
				printf("Couldn't parse HTTP data.  Closing conn.\n");
				closeConn(cp, PS_PROTOCOL_FAIL);
				continue; /* Next connection */
			}
			if(bReadMore)
			{
				goto READ_MORE;
			}
			if (cp->parsebuf != NULL) {
				/* Test for one of our custom testing messages */
				if (strncmp((const char*)cp->parsebuf,
						"MATRIX_SHUTDOWN", 15) == 0) {
					g_exitFlag = 1;
					printf("Got MATRIX_SHUTDOWN.  Exiting\n");
					closeConn(cp, PS_ARG_FAIL);
					continue;	/* Next connection */
				}
				
			}
			/* reply to /bytes?<byte count> syntax */
			if (len > 11 &&
					strncmp((char *)buf, "GET /bytes?", 11) == 0) {
				cp->bytes_requested = atoi((char *)buf + 11);
				if (cp->bytes_requested <
						strlen((char *)g_httpResponseHdr) ||
						cp->bytes_requested > 1073741824) {
        			cp->bytes_requested =
						strlen((char *)g_httpResponseHdr);
    			}
			}
			if (rc == HTTPS_COMPLETE) {
				if (httpWriteResponse(cp) < 0) {
					closeConn(cp, PS_PROTOCOL_FAIL);
					continue; /* Next connection */
				}
				/* For HTTP, we assume no pipelined requests, so we 
				 close after parsing a single HTTP request */
				/* Ignore return of closure alert, it's optional */
				closeConn(cp, PS_SUCCESS);
				continue; /* Next connection */
			}
		} /*  readfd handling */
	}	/* connection loop */
	return PS_SUCCESS;
}

/******************************************************************************/
/*
	Create an HTTP response and encode it to the SSL buffer
 */
static int32 httpWriteResponse(httpConn_t *conn)
{
	char	*buf;
	ssl_t			*cp;
	int32			len, rc;

		
	cp = conn->ssl;
	if (conn->bytes_requested) {
		/* The /bytes? syntax */
		while (conn->bytes_sent < conn->bytes_requested) {
			len = conn->bytes_requested - conn->bytes_sent;
			if (len > RECV_BUF_LEN) {
				len = RECV_BUF_LEN;
    		}
			buf = RECV_BUF;
			memset(buf, 'J', len);
			rc = tls_ssl_server_send(cp, conn->fd, buf, len, 0);
			if(SOCKET_ERROR == rc)
			{
				return -1;
			}
    	}
		return 0;
	}

	/* Usual reply */
	buf = (char *)g_httpResponseHdr;
	len = strlen((char *)g_httpResponseHdr) + 1;
	rc = tls_ssl_server_send(cp, conn->fd, buf, len, 0);
	if(SOCKET_ERROR == rc)
	{
		return -1;
	}
	return 0;
}

static int32 httpBasicParse(httpConn_t *cp, unsigned char *buf, uint32 len, 
		int32 trace)
{
	unsigned char	*c, *end, *tmp;
	int32	l;

	/*
		SSL/TLS can provide zero length records, which we just ignore here
		because the code below assumes we have at least one byte
	*/
	if (len == 0) {
		return HTTPS_PARTIAL;
	}

	c = buf;
	end = c + len;
/*
	If we have an existing partial HTTP buffer, append to it the data in buf
	up to the first newline, or 'len' data, if no newline is in buf.
 */
	if (cp->parsebuf != NULL) {
		for (tmp = c; c < end && *c != '\n'; c++);
		/* We want c to point to 'end' or to the byte after \r\n */
		if (*c == '\n') {
			c++;
		}
		l = (int32)(c - tmp);
		if (l > HTTPS_BUFFER_MAX) {
			return HTTPS_ERROR;
		}
		cp->parsebuf = tls_mem_realloc(cp->parsebuf, l + cp->parsebuflen);
		memcpy(cp->parsebuf + cp->parsebuflen, tmp, l);
		cp->parsebuflen += l;
		/* Parse the data out of the saved buffer first */
		c = cp->parsebuf;
		end = c + cp->parsebuflen;
		/* We've "moved" some data from buf into parsebuf, so account for it */
		buf += l;
		len -= l;
	}
	
L_PARSE_LINE:
	for (tmp = c; c < end && *c != '\n'; c++);
	if (c < end) {
		if (*(c - 1) != '\r') {
			return HTTPS_ERROR;
		}
		/* If the \r\n started the line, we're done reading headers */
		if (*tmp == '\r' && (tmp + 1 == c)) {
/*
			if ((c + 1) != end) {
				printf("HTTP data parsing not supported, ignoring.\n");
			}
*/
			if (cp->parsebuf != NULL) {
				tls_mem_free(cp->parsebuf); cp->parsebuf = NULL;
				cp->parsebuflen = 0;
				if (len != 0) {
					printf("HTTP data parsing not supported, ignoring.\n");
				}
			}
			if (trace) printf("RECV COMPLETE HTTP MESSAGE\n");
			return HTTPS_COMPLETE;
		}
	} else {
		/* If parsebuf is non-null, we have already saved it */
		if (cp->parsebuf == NULL && (l = (int32)(end -tmp)) > 0) {
			cp->parsebuflen = l;
			cp->parsebuf = tls_mem_alloc(cp->parsebuflen);
			memcpy(cp->parsebuf, tmp, cp->parsebuflen);
		}
		return HTTPS_PARTIAL;
	}
	*(c - 1) = '\0';	/* Replace \r with \0 just for printing */
	if (trace) printf("RECV PARSED: [%s]\n", (char *)tmp);
	/* Finished parsing the saved buffer, now start parsing from incoming buf */
	if (cp->parsebuf != NULL) {
		tls_mem_free(cp->parsebuf); cp->parsebuf = NULL;
		cp->parsebuflen = 0;
		c = buf;
		end = c + len;
	} else {
		c++;	/* point c to the next char after \r\n */
	}		
	goto L_PARSE_LINE;
}


/******************************************************************************/
/*
	non-blocking SSL server
	Initialize MatrixSSL and sockets layer, and loop on select
 */
int32 server_idle(int proto_ver)
{
	tls_ssl_key_t		*keys = NULL;
	SOCKET			lfd = INVALID_SOCKET;
	unsigned char	*CAstream;
	int32			err, rc, CAstreamLen;


	keys = NULL;
	DLListInit(&g_conns);
	g_exitFlag = 0;
	lfd = INVALID_SOCKET;

	if ((rc=tls_ssl_server_init((void*)proto_ver)) < 0) {
		printf("tls_ssl_server_init key init failure.  Exiting\n");
		return rc;
	}
	
/*
	In-memory based keys
	Build the CA list first for potential client auth usage
*/
	CAstreamLen = 0;
#ifdef USE_RSA
	CAstreamLen += sizeof(RSACAS);
#endif
	CAstream = tls_mem_alloc(CAstreamLen);
	
	CAstreamLen = 0;
#ifdef USE_RSA
	memcpy(CAstream, RSACAS, sizeof(RSACAS));
	CAstreamLen += sizeof(RSACAS);
#endif
		
	if (tls_ssl_server_load_keys(&keys,	RSA1024, sizeof(RSA1024),
		   RSA1024KEY, sizeof(RSA1024KEY), CAstream, CAstreamLen, KEY_RSA) < 0) {
		printf("tls_ssl_server_load_keys key init failure.  Exiting\n");
		tls_mem_free(CAstream);
		goto L_EXIT;
	}
	tls_mem_free(CAstream);

	/* Create the listening socket that will accept incoming connections */
	if ((lfd = socketListen(HTTPS_PORT, &err)) == INVALID_SOCKET) {
		printf("Can't listen on port %d\n", HTTPS_PORT);
		goto L_EXIT;
	}

	/* Main select loop to handle sockets events */
	while (!g_exitFlag) {
		selectLoop((tls_ssl_t *)keys, lfd);
	}

L_EXIT:
	if (lfd != INVALID_SOCKET) close(lfd);
	tls_ssl_server_close(keys);
	
	return 0;
}

/******************************************************************************/
/*
	Close a socket and tls_mem_free associated SSL context and buffers
 */
static void closeConn(httpConn_t *cp, int32 reason)
{	
	DLListRemove(&cp->List);

	tls_ssl_server_close_conn(cp->ssl, cp->fd);
	
	if (cp->parsebuf != NULL) {
		tls_mem_free(cp->parsebuf);
		cp->parsebuflen = 0;
	}
		
	if (cp->fd != INVALID_SOCKET) {
		close(cp->fd);
	}
	if (reason >= 0) {
		printf("=== Closing Client %d ===\n", cp->fd);
		printf("=== Closing Client on Reason %d  ===\n", reason);
	} else {
		printf("=== Closing Client %d on Error ===\n", cp->fd);
		printf("=== Closing Client on Error %d  ===\n", reason);
	}
	tls_mem_free(cp);
}

/******************************************************************************/
/*
	Establish a listening socket for incomming connections
 */
static SOCKET socketListen(short port, int32 *err)
{
	struct sockaddr_in	addr;
	SOCKET				fd;
	
	if ((fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		printf("Error creating listen socket\n");
		*err = SOCKET_ERRNO;
		return INVALID_SOCKET;
	}
	
	setSocketOptions(fd);
	
	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);
	addr.sin_addr.s_addr = INADDR_ANY;
	if (bind(fd, (struct sockaddr *) &addr, sizeof(addr)) < 0) {
		printf("Can't bind socket. Port in use or insufficient privilege\n");
		*err = SOCKET_ERRNO;
		return INVALID_SOCKET;
	}
	if (listen(fd, BACKLOG) < 0) {
		printf("Error listening on socket\n");
		*err = SOCKET_ERRNO;
		return INVALID_SOCKET;
	}
	printf("Listening on port %d\n", port);
	return fd;
}

/******************************************************************************/
/*
	Make sure the socket is not inherited by exec'd processes
	Set the REUSE flag to minimize the number of sockets in TIME_WAIT
	Then we set REUSEADDR, NODELAY and NONBLOCK on the socket
*/
static void setSocketOptions(SOCKET fd)
{
	int32 rc;
	rc = 1;
	setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (char *)&rc, sizeof(rc));
}

#endif

static void demo_ssl_server_task(void *sdata);

int CreateSSLServerDemoTask(char *buf)
{
	tls_os_queue_create(&demo_ssl_server_q, DEMO_QUEUE_SIZE);
	tls_os_task_create(NULL, NULL,
			demo_ssl_server_task,
                    (void *)NULL,
                    (void *)DemoSSLServerTaskStk,          /* 任务栈的起始地址 */
                    DEMO_SSL_SERVER_TASK_SIZE * sizeof(u32), /* 任务栈的大小     */
                    DEMO_SSL_SERVER_TASK_PRIO,
                    0);

	return WM_SUCCESS;
}

static void ssl_server_net_status_changed_event(u8 status )
{
	switch(status)
	{
		case NETIF_WIFI_JOIN_FAILED:
			tls_os_queue_send(demo_ssl_server_q, (void *)DEMO_MSG_WJOIN_FAILD, 0);
			break;
		case NETIF_WIFI_JOIN_SUCCESS:
			tls_os_queue_send(demo_ssl_server_q, (void *)DEMO_MSG_WJOIN_SUCCESS, 0);
			break;
		case NETIF_IP_NET_UP:
			tls_os_queue_send(demo_ssl_server_q, (void *)DEMO_MSG_SOCKET_CREATE, 0);
			break;
		default:
			break;
	}
}

static void demo_ssl_server_task(void *sdata)
{
	void *msg;
	struct tls_ethif * ethif = tls_netif_get_ethif();
	
	printf("\nssl server task\n");
	if(ethif->status)	//已经在网
	{
		tls_os_queue_send(demo_ssl_server_q, (void *)DEMO_MSG_SOCKET_CREATE, 0);
	}
	else
	{
		struct tls_param_ip ip_param;
		
		tls_param_get(TLS_PARAM_ID_IP, &ip_param, TRUE);
		ip_param.dhcp_enable = TRUE;
		tls_param_set(TLS_PARAM_ID_IP, &ip_param, TRUE);
		tls_wifi_set_oneshot_flag(1);		/*一键配置使能*/
		printf("\nwait one shot......\n");
	}
	tls_netif_add_status_event(ssl_server_net_status_changed_event);
	for(;;) 
	{
		tls_os_queue_receive(demo_ssl_server_q, (void **)&msg, 0, 0);
	//	printf("\n raw s c msg =%d\n",msg);
		switch((u32)msg)
		{
			case DEMO_MSG_WJOIN_SUCCESS:
				break;
				
			case DEMO_MSG_SOCKET_CREATE:
				server_idle(3);
				break;
				
			case DEMO_MSG_WJOIN_FAILD:
				break;

			case DEMO_MSG_SOCKET_RECEIVE_DATA:
				break;

			case DEMO_MSG_SOCKET_ERR:
				break;
			default:
				break;
		}
	}

}

#endif /* DEMO_SSL_SERVER */

