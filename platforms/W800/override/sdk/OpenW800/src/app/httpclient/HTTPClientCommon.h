
///////////////////////////////////////////////////////////////////////////////
//
// Module Name:                                                                
//   CmsiHTTPClientCommon.h                                                    
//                                                                             
// Abstract: Coomon structs and types for the HTTP protocol API                
// Author:	Eitan Michaelso                                                     
// Version:  1.0                                                               
//                                                                             
///////////////////////////////////////////////////////////////////////////////

#ifndef _HTTPCLIENT_PROTOCOL_H_
#define _HTTPCLIENT_PROTOCOL_H_
#include "wm_type_def.h"

#ifdef __cplusplus 
extern "C" { 
#endif

    // Global default sizes
#define HTTP_CLIENT_MAX_URL_LENGTH          512         // Maximum length for an HTTP Url parameter

    // HTTP Session flags (Public flags)
#define HTTP_CLIENT_FLAG_KEEP_ALIVE         0x00000001 // Set the keep alive header
#define HTTP_CLIENT_FLAG_SEND_CHUNKED       0x00000002 // The outgoing should chunked
#define HTTP_CLIENT_FLAG_NO_CACHE           0x00000004 // Set the no cache header
#define HTTP_CLIENT_FLAG_ASYNC              0x00000008 // Currently not implemented 

    // HTTP status internal flags
#define HTTP_CLIENT_STATE_PRE_INIT          0x00000000  // Starting stage
#define HTTP_CLIENT_STATE_INIT              0x00000001  // API was initialized (memory was allocated)
#define HTTP_CLIENT_STATE_URL_PARSED        0x00000002  // Url was parsed
#define HTTP_CLIENT_STATE_HOST_CONNECTED    0x00000004  // HEAD verb was sent
#define HTTP_CLIENT_STATE_HEAD_SENT         0x00000008  // Post verb was sent
#define HTTP_CLIENT_STATE_POST_SENT         0x00000010  // HTTP requet was sent
#define HTTP_CLIENT_STATE_REQUEST_SENT      0x00000020  // HTTP request was sent
#define HTTP_CLIENT_STATE_HEADERS_RECIVED   0x00000040  // Headers ware recived from the server
#define HTTP_CLIENT_STATE_HEADERS_PARSED    0x00000080  // HTTP headers ware parsed
#define HTTP_CLIENT_STATE_HEADERS_OK        0x00000100  // Headers  status was OK
  
#endif // _HTTPCLIENT_PROTOCOL_H_
