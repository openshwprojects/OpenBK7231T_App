/******************************************************************************
* Copyright © 2004 Altera Corporation, San Jose, California, USA.             *
* All rights reserved. All use of this software and documentation is          *
* subject to the License Agreement located at the end of this file below.     *
*******************************************************************************
* Author - PRR/JRK                                                            *
*                                                                             *
* File: http.h                                                                *
*                                                                             *
* Headers for our "basic" implementation of HTTP. Please note this is not a   *
* complete implementation only enough for our demo web server.                *
*                                                                             *
* Please refer to file readme.txt for notes on this software example.         *
******************************************************************************/
#ifndef __HTTP_H__
#define __HTTP_H__

#define   HTTP_RX_BUF_SIZE      1024  /* Receive buffer size */
#define   HTTP_TX_BUF_SIZE      8192  /* Transmission buffer size */
#define   HTTP_URI_SIZE         80    /* Max size of a URI *URL) string */
#define   HTTP_KEEP_ALIVE_COUNT 20    /* Max number of files per connection */
#define   HTTP_KEEP_ALIVE_TIME  5000  /* TCP connection keep-alive time (ms) */
#define   HTTP_PORT             80    /* TCP port number to listen on */
#define   HTTP_NUM_CONNECTIONS  6     /* Maximum concurrent HTTP connections */

#define   HTTP_DEFAULT_FILE       "/index.html"
#define   HTTP_VERSION_STRING     "HTTP/1.1 "
#define   HTTP_OK                 200
#define   HTTP_OK_STRING          "200 OK\r\n"
#define   HTTP_NO_CONTENT_STRING  "204 No Content\r\n"
#define   SOAP_ENV                "<env:Envelope\nxmlns:env=\"http://www.w3.org/2001/12/soap-envelope\" >\r\n"
#define   SOAP_URI                "<env:Body xmlns:services=\"some URI\">\r\n"
#define   SOAP_FINISH             "</env:Envelope>"
#define   HTTP_NOT_FOUND          404
#define   HTTP_NOT_FOUND_STRING   "404 Not Found\r\n"
#define   HTTP_NOT_FOUND_FILE     "/not_found.html"
#define   HTTP_CONTENT_TYPE       "Content-Type: "
#define   HTTP_CONTENT_TYPE_HTML  "text/html\r\n"
#define   get   "image/jpg\r\n"
#define   HTTP_CONTENT_TYPE_GIF   "image/gif\r\n"
#define   HTTP_CONTENT_TYPE_JS    "application/x-javascript\r\n"
#define   HTTP_CONTENT_TYPE_CSS   "text/css\r\n"
#define   HTTP_CONTENT_TYPE_SWF   "application/x-shockwave-flash\r\n"
#define   HTTP_CONTENT_LENGTH     "Content-Length: "
#define   HTTP_KEEP_ALIVE         "Connection: Keep-Alive\r\n"
#define   HTTP_CLOSE              "Connection: close\r\n"
#define   HTTP_CR_LF              "\r\n"
#define   HTTP_END_OF_HEADERS     "\r\n\r\n"

typedef struct http_socket
{
  enum      { READY, PROCESS, DATA, COMPLETE, RESET, CLOSE } state; 
  enum      { UNKNOWN=0, GET, POST } action;
  enum      { COUNTED=0,CHUNKED,UNKNOWN_ENC } rx_encoding;
  int       fd;                   /* Socket descriptor */
  int       close;                /* Close the connection after we're done? */
  int       content_length;       /* Extracted(??) content length */
  int       keep_alive_count;     /* No. of files tx'd in single connection */
  int       file_length;          /* Length of the current file being sent */
  int       data_sent;            /* Number of bytes already sent */
  FILE*     file_handle;          /* File handle for file we're sending */
  alt_u32   activity_time;        /* Time of last HTTP activity */
  alt_u8*   rx_rd_pos;            /* position we've read up to */
  alt_u8*   rx_wr_pos;            /* position we've written up to */
  alt_u8*   rx_buffer;            /* pointer to global RX buffer */
  alt_u8*   tx_buffer;            /* pointer to global TX buffer */
  alt_u8    uri[HTTP_URI_SIZE];   /* URI buffer */
}http_conn;

#endif /* __HTTP_H__ */

/******************************************************************************
*                                                                             *
* License Agreement                                                           *
*                                                                             *
* Copyright (c) 2004 Altera Corporation, San Jose, California, USA.           *
* All rights reserved.                                                        *
*                                                                             *
* Permission is hereby granted, free of charge, to any person obtaining a     *
* copy of this software and associated documentation files (the "Software"),  *
* to deal in the Software without restriction, including without limitation   *
* the rights to use, copy, modify, merge, publish, distribute, sublicense,    *
* and/or sell copies of the Software, and to permit persons to whom the       *
* Software is furnished to do so, subject to the following conditions:        *
*                                                                             *
* The above copyright notice and this permission notice shall be included in  *
* all copies or substantial portions of the Software.                         *
*                                                                             *
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR  *
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,    *
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE *
* AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER      *
* LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING     *
* FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER         *
* DEALINGS IN THE SOFTWARE.                                                   *
*                                                                             *
* This agreement shall be governed in all respects by the laws of the State   *
* of California and by the laws of the United States of America.              *
* Altera does not recommend, suggest or require that this reference design    *
* file be used in conjunction or combination with any other product.          *
******************************************************************************/
