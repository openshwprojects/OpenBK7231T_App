/****************************************************************************
**
**  Name        gki_int.h
**
**  Function    This file contains GKI private definitions
**
**
**  Copyright (c) 1999-2004, WIDCOMM Inc., All Rights Reserved.
**  Proprietary and confidential.
**
*****************************************************************************/
#ifndef GKI_INT_H
#define GKI_INT_H

#include "gki_common.h"

/**********************************************************************
** OS specific definitions
*/
typedef struct {
    /* OS specific variables here */
    TASKPTR task_ptr[GKI_MAX_TASKS];
    uint16_t flag;
} tGKI_OS;


/* Contains common control block as well as OS specific variables */
typedef struct {
    tGKI_OS     os;
    tGKI_COM_CB com;
} tGKI_CB;


#ifdef __cplusplus
extern "C" {
#endif

#if GKI_DYNAMIC_MEMORY == FALSE
GKI_API extern tGKI_CB  gki_cb;
#else
GKI_API extern tGKI_CB *gki_cb_ptr;
#define gki_cb (*gki_cb_ptr)
#endif

#ifdef __cplusplus
}
#endif

#endif
