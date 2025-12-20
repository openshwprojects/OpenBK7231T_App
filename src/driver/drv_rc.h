#ifndef __DRV_RC_H__
#define __DRV_RC_H__

#ifdef __cplusplus
extern "C" {
#endif

void DRV_RC_Init();
void DRV_RC_RunFrame();
void RC_AppendInformationToHTTPIndexPage(http_request_t *request, int bPreState);

#ifdef __cplusplus
}

#endif
#endif

