
#ifndef _HTTP_CLIENT_STRING
#define _HTTP_CLIENT_STRING

#include "HTTPClientWrapper.h" // Cross platform support
#include "HTTPClient.h" 

///////////////////////////////////////////////////////////////////////////////
//
// Section      : HTTP Api global definitions
// Last updated : 01/09/2005
//
///////////////////////////////////////////////////////////////////////////////
int                        stricmp(const char *s1, const char *s2) ;
BOOL                    HTTPStrInsensitiveCompare   (const CHAR *pSrc, const CHAR* pDest, UINT32 nLength);
BOOL                    HTTPStrSearch               (CHAR *pSrc, CHAR *pSearched, UINT32 nOffset, UINT32 nScope,HTTP_PARAM *HttpParam);
CHAR                    HTTPStrExtract              (CHAR *pParam,UINT32 nOffset,CHAR Restore);
CHAR*                   HTTPStrCaseStr              (const CHAR *pSrc, UINT32 nSrcLength, const CHAR *pFind); 
CHAR*                   HTTPStrGetToken             (CHAR *pSrc, UINT32 nSrcLength, CHAR *pDest, UINT32 *nDestLength);
UINT32                  HTTPStrGetDigestToken       (HTTP_PARAM pParamSrc, CHAR *pSearched, HTTP_PARAM *pParamDest);
UINT32                  HTTPStrHToL                 (CHAR * s); 
CHAR*                   HTTPStrLToH                 (CHAR * dest,UINT32 nSrc);        
#endif
