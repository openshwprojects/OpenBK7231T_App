/******************************************************************************
 *
 *  Copyright (C) 2000-2012 Broadcom Corporation
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at:
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 ******************************************************************************/

/******************************************************************************
 *
 *  This module contains the routines that initialize the stack components.
 *  It must be called before the BTU task is started.
 *
 ******************************************************************************/

#include "bt_target.h"
#include <string.h>
#include <assert.h>

#ifndef BTA_INCLUDED
#define BTA_INCLUDED FALSE
#endif

/* Include initialization functions definitions */
#include "port_api.h"

#if (defined(BNEP_INCLUDED) && BNEP_INCLUDED == TRUE)
#include "bnep_api.h"
#endif

#include "gap_api.h"

#if (defined(PAN_INCLUDED) && PAN_INCLUDED == TRUE)
#include "pan_api.h"
#endif

#include "avrc_api.h"

#if (defined(A2D_INCLUDED) && A2D_INCLUDED == TRUE)
#include "a2d_api.h"
#endif

#if (defined(HID_HOST_INCLUDED) && HID_HOST_INCLUDED == TRUE)
#include "hidh_api.h"
#endif

#if (defined(MCA_INCLUDED) && MCA_INCLUDED == TRUE)
#include "mca_api.h"
#endif

#if (defined(BLE_INCLUDED) && BLE_INCLUDED == TRUE)
#include "gatt_api.h"
#if (defined(SMP_INCLUDED) && SMP_INCLUDED == TRUE)
#include "smp_api.h"
#endif
#endif

/***** BTA Modules ******/
#if BTA_INCLUDED == TRUE && BTA_DYNAMIC_MEMORY == TRUE
#include "bta_api.h"
#include "bta_sys.h"
#include "bta_ag_int.h"

#if BTA_HS_INCLUDED == TRUE
#include "bta_hs_int.h"
#endif

#include "bta_dm_int.h"

#if BTA_AR_INCLUDED==TRUE
#include "bta_ar_int.h"
#endif
#if BTA_AV_INCLUDED==TRUE
#include "bta_av_int.h"
#endif

#if BTA_HH_INCLUDED==TRUE
#include "bta_hh_int.h"
#endif

#if BTA_JV_INCLUDED==TRUE
#include "bta_jv_int.h"
tBTA_JV_CB *bta_jv_cb_ptr = NULL;
#endif

#if BTA_HL_INCLUDED == TRUE
#include "bta_hl_int.h"
#endif

#if BTA_GATT_INCLUDED == TRUE
#include "bta_gattc_int.h"
#include "bta_gatts_int.h"
#endif

#if BTA_PAN_INCLUDED==TRUE
#include "bta_pan_int.h"
#endif

#include "bta_sys_int.h"

#endif /* BTA_INCLUDED */

/*****************************************************************************
**                          F U N C T I O N S                                *
******************************************************************************/

/*****************************************************************************
**
** Function         BTE_InitStack
**
** Description      Initialize control block memory for each component.
**
**                  Note: The core stack components must be called
**                      before creating the BTU Task.  The rest of the
**                      components can be initialized at a later time if desired
**                      as long as the component's init function is called
**                      before accessing any of its functions.
**
** Returns          void
**
******************************************************************************/
void BTE_InitStack(void)
{
    /* Initialize the optional stack components */
    RFCOMM_Init();
    /**************************
    ** BNEP and its profiles **
    ***************************/
#if (defined(BNEP_INCLUDED) && BNEP_INCLUDED == TRUE)
    BNEP_Init();
#if (defined(PAN_INCLUDED) && PAN_INCLUDED == TRUE)
    PAN_Init();
#endif  /* PAN */
#endif  /* BNEP Included */
    /**************************
    ** AVDT and its profiles **
    ***************************/
#if (defined(A2D_INCLUDED) && A2D_INCLUDED == TRUE)
    A2D_Init();
#endif  /* AADP */
    AVRC_Init();
    /***********
    ** Others **
    ************/
    GAP_Init();
#if (defined(HID_HOST_INCLUDED) && HID_HOST_INCLUDED == TRUE)
    HID_HostInit();
#endif
#if (defined(MCA_INCLUDED) && MCA_INCLUDED == TRUE)
    MCA_Init();
#endif
    /****************
    ** BTA Modules **
    *****************/
#if (BTA_INCLUDED == TRUE && BTA_DYNAMIC_MEMORY == TRUE)

    if((bta_sys_cb_ptr = (tBTA_SYS_CB *)GKI_os_malloc(sizeof(tBTA_SYS_CB))) == NULL) {
        assert(0);
    }

    if((bta_dm_cb_ptr = (tBTA_DM_CB *)GKI_os_malloc(sizeof(tBTA_DM_CB))) == NULL) {
        assert(0);
    }

    if((bta_dm_search_cb_ptr = (tBTA_DM_SEARCH_CB *)GKI_os_malloc(sizeof(tBTA_DM_SEARCH_CB))) == NULL) {
        assert(0);
    }

    if((bta_dm_di_cb_ptr = (tBTA_DM_DI_CB *)GKI_os_malloc(sizeof(tBTA_DM_DI_CB))) == NULL) {
        assert(0);
    }

    wm_memset((void *)bta_sys_cb_ptr, 0, sizeof(tBTA_SYS_CB));
    wm_memset((void *)bta_dm_cb_ptr, 0, sizeof(tBTA_DM_CB));
    wm_memset((void *)bta_dm_search_cb_ptr, 0, sizeof(tBTA_DM_SEARCH_CB));
    wm_memset((void *)bta_dm_di_cb_ptr, 0, sizeof(tBTA_DM_DI_CB));
    //wm_memset((void *)bta_prm_cb_ptr, 0, sizeof(tBTA_PRM_CB));
#if BTA_HFP_HSP_AG_INCLUDED == TRUE

    if((bta_ag_cb_ptr = (tBTA_AG_CB *)GKI_os_malloc(sizeof(tBTA_AG_CB))) == NULL) {
        assert(0);
    }

    wm_memset((void *)bta_ag_cb_ptr, 0, sizeof(tBTA_AG_CB));
#endif
#if BTA_HS_INCLUDED == TRUE

    if((bta_hs_cb_ptr = (tBTA_HS_CB *)GKI_os_malloc(sizeof(tBTA_HS_CB))) == NULL) {
        assert(0);
    }

    wm_memset((void *)bta_hs_cb_ptr, 0, sizeof(tBTA_HS_CB));
#endif
#if BTA_SDP_INCLUDED == TRUE

    if((bta_sdp_cb_ptr = (tBTA_SDP_CB *)GKI_os_malloc(sizeof(tBTA_SDP_CB))) == NULL) {
        assert(0);
    }

    memset((void *)bta_sdp_cb_ptr, 0, sizeof(tBTA_SDP_CB));
#endif
#if (defined BTA_JV_INCLUDED && BTA_JV_INCLUDED == TRUE)

    if((bta_jv_cb_ptr = (tBTA_JV_CB *)GKI_os_malloc(sizeof(tBTA_JV_CB))) == NULL) {
        assert(0);
    }

    memset((void *)bta_jv_cb_ptr, 0, sizeof(tBTA_JV_CB));
#endif //JV
#if BTA_AR_INCLUDED==TRUE

    if((bta_ar_cb_ptr = (tBTA_AR_CB *)GKI_os_malloc(sizeof(tBTA_AR_CB))) == NULL) {
        assert(0);
    }

    wm_memset((void *)bta_ar_cb_ptr, 0, sizeof(tBTA_AR_CB));
#endif
#if BTA_AV_INCLUDED==TRUE

    if((bta_av_cb_ptr = (tBTA_AV_CB *)GKI_os_malloc(sizeof(tBTA_AV_CB))) == NULL) {
        assert(0);
    }

    wm_memset((void *)bta_av_cb_ptr, 0, sizeof(tBTA_AV_CB));
#endif
#if BTA_HH_INCLUDED==TRUE

    if((bta_hh_cb_ptr = (tBTA_HH_CB *)GKI_os_malloc(sizeof(tBTA_HH_CB))) == NULL) {
        assert(0);
    }

    wm_memset((void *)bta_hh_cb_ptr, 0, sizeof(tBTA_HH_CB));
#endif
#if BTA_HL_INCLUDED==TRUE

    if((bta_hl_cb_ptr = (tBTA_HL_CB *)GKI_os_malloc(sizeof(tBTA_HL_CB))) == NULL) {
        assert(0);
    }

    wm_memset((void *)bta_hl_cb_ptr, 0, sizeof(tBTA_HL_CB));
#endif
#if BTA_GATT_INCLUDED==TRUE

    if((bta_gattc_cb_ptr = (tBTA_GATTC_CB *)GKI_os_malloc(sizeof(tBTA_GATTC_CB))) == NULL) {
        assert(0);
    }

    wm_memset((void *)bta_gattc_cb_ptr, 0, sizeof(tBTA_GATTC_CB));

    if((bta_gatts_cb_ptr = (tBTA_GATTS_CB *)GKI_os_malloc(sizeof(tBTA_GATTS_CB))) == NULL) {
        assert(0);
    }

    wm_memset((void *)bta_gatts_cb_ptr, 0, sizeof(tBTA_GATTS_CB));
#endif
#if BTA_PAN_INCLUDED==TRUE
    wm_memset((void *)bta_pan_cb_ptr, 0, sizeof(tBTA_PAN_CB));
#endif
#endif /* BTA_INCLUDED == TRUE */
}


/*****************************************************************************
**
** Function         BTE_DeinitStack
**
** Description      Deinitialize control block memory for each component.
**
**                  Note: This API must be called
**                      after freeing the BTU Task.
**
** Returns          void
**
******************************************************************************/
void BTE_DeinitStack(void)
{
    //BTA Modules
#if (BTA_INCLUDED == TRUE && BTA_DYNAMIC_MEMORY == TRUE)
#if BTA_GATT_INCLUDED==TRUE
    GKI_os_free(bta_gatts_cb_ptr);
    bta_gatts_cb_ptr = NULL;
    GKI_os_free(bta_gattc_cb_ptr);
    bta_gattc_cb_ptr = NULL;
#endif
#if BTA_HH_INCLUDED==TRUE
    GKI_os_free(bta_hh_cb_ptr);
    bta_hh_cb_ptr = NULL;
#endif
#if BTA_AV_INCLUDED==TRUE
    GKI_os_free(bta_av_cb_ptr);
    bta_av_cb_ptr = NULL;
#endif
#if BTA_AR_INCLUDED==TRUE
    GKI_os_free(bta_ar_cb_ptr);
    bta_ar_cb_ptr = NULL;
#endif
#if BTA_SDP_INCLUDED == TRUE
    GKI_os_free(bta_sdp_cb_ptr);
    bta_sdp_cb_ptr = NULL;
#endif
#if (defined BTA_JV_INCLUDED && BTA_JV_INCLUDED == TRUE)
    GKI_os_free(bta_jv_cb_ptr);
    bta_jv_cb_ptr = NULL;
#endif //JV
    GKI_os_free(bta_dm_di_cb_ptr);
    bta_dm_di_cb_ptr = NULL;
    GKI_os_free(bta_dm_search_cb_ptr);
    bta_dm_search_cb_ptr = NULL;
    GKI_os_free(bta_dm_cb_ptr);
    bta_dm_cb_ptr = NULL;
#if BTA_HFP_HSP_AG_INCLUDED == TRUE
    GKI_os_free(bta_ag_cb_ptr);
    bta_ag_cb_ptr = NULL;
#endif
#endif // BTA_INCLUDED == TRUE
#if (defined(AVCT_INCLUDED) && AVCT_INCLUDED == TRUE && AVCT_DYNAMIC_MEMORY == TRUE)
    GKI_os_free(avct_cb_ptr);
    avct_cb_ptr = NULL;
#endif
#if (defined(AVDT_INCLUDED) && AVDT_INCLUDED == TRUE && AVDT_DYNAMIC_MEMORY == TRUE)
    GKI_os_free(avdt_cb_ptr);
    avdt_cb_ptr = NULL;
#endif
#if (defined(AVRC_INCLUDED) && AVRC_INCLUDED == TRUE)
    AVRC_Deinit();
#endif
#if (defined(A2D_INCLUDED) && A2D_INCLUDED == TRUE)
    A2D_Deinit();
#endif
    RFCOMM_Deinit();
}

