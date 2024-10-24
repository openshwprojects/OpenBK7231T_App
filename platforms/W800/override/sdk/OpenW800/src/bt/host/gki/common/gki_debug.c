/******************************************************************************
 *
 *  Copyright (C) 1999-2012 Broadcom Corporation
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

#include "gki_int.h"

#if (GKI_DEBUG == TRUE)

const int8_t *const OSTaskStates[] = {
    (int8_t *)"DEAD",   /* 0 */
    (int8_t *)"REDY",   /* 1 */
    (int8_t *)"WAIT",   /* 2 */
    (int8_t *)"",
    (int8_t *)"DELY",   /* 4 */
    (int8_t *)"",
    (int8_t *)"",
    (int8_t *)"",
    (int8_t *)"SUSP",   /* 8 */
};


/*******************************************************************************
**
** Function         GKI_PrintBufferUsage
**
** Description      Displays Current Buffer Pool summary
**
** Returns          void
**
*******************************************************************************/
void GKI_PrintBufferUsage(uint8_t *p_num_pools, uint16_t *p_cur_used)
{
    int i;
    FREE_QUEUE_T    *p;
    uint8_t   num = gki_cb.com.curr_total_no_of_pools;
    uint16_t   cur[GKI_NUM_TOTAL_BUF_POOLS];
    GKI_TRACE("");
    GKI_TRACE("--- GKI Buffer Pool Summary (R - restricted, P - public) ---");
    GKI_TRACE("POOL     SIZE  USED  MAXU  TOTAL");
    GKI_TRACE("------------------------------");

    for(i = 0; i < gki_cb.com.curr_total_no_of_pools; i++) {
        p = &gki_cb.com.freeq[i];

        if((1 << i) & gki_cb.com.pool_access_mask) {
            GKI_TRACE("%02d: (R), %4d, %3d, %3d, %3d",
                      i, p->size, p->cur_cnt, p->max_cnt, p->total);
        } else {
            GKI_TRACE("%02d: (P), %4d, %3d, %3d, %3d",
                      i, p->size, p->cur_cnt, p->max_cnt, p->total);
        }

        cur[i] = p->cur_cnt;
    }

    if(p_num_pools) {
        *p_num_pools = num;
    }

    if(p_cur_used) {
        wm_memcpy(p_cur_used, cur, num * 2);
    }
}

/*******************************************************************************
**
** Function         GKI_PrintBuffer
**
** Description      Called internally by OSS to print the buffer pools
**
** Returns          void
**
*******************************************************************************/
void GKI_PrintBuffer(void)
{
    uint16_t i;

    for(i = 0; i < GKI_NUM_TOTAL_BUF_POOLS; i++) {
        GKI_TRACE("pool:%4u free %4u cur %3u max %3u  total%3u", i, gki_cb.com.freeq[i].size,
                  gki_cb.com.freeq[i].cur_cnt, gki_cb.com.freeq[i].max_cnt, gki_cb.com.freeq[i].total);
    }
}

/*******************************************************************************
**
** Function         gki_calc_stack
**
** Description      This function tries to calculate the amount of
**                  stack used by looking non magic num. Magic num is consider
**                  the first byte in the stack.
**
** Returns          the number of unused byte on the stack. 4 in case of stack overrun
**
*******************************************************************************/
uint16_t gki_calc_stack(uint8_t task)
{
    int    j, stacksize;
    uint32_t MagicNum;
    uint32_t *p;
    stacksize = (int) gki_cb.com.OSStackSize[task];
    p = (uint32_t *)gki_cb.com.OSStack[task];   /* assume stack is aligned, */
    MagicNum = *p;

    for(j = 0; j < stacksize; j++) {
        if(*p++ != MagicNum) {
            break;
        }
    }

    return (j * sizeof(uint32_t));
}

/*******************************************************************************
**
** Function         GKI_print_task
**
** Description      Print task stack usage.
**
** Returns          void
**
*******************************************************************************/
void GKI_print_task(void)
{
#ifdef _BT_WIN32
    GKI_TRACE("Service not available under insight");
#else
    uint8_t TaskId;
    GKI_TRACE("TID TASKNAME STATE FREE_STACK  STACK");

    for(TaskId = 0; TaskId < GKI_MAX_TASKS; TaskId++) {
        if(gki_cb.com.OSRdyTbl[TaskId] != TASK_DEAD) {
            GKI_TRACE("%2u   %-8s %-5s  0x%04X     0x%04X Bytes",
                      (uint16_t)TaskId,  gki_cb.com.OSTName[TaskId],
                      OSTaskStates[gki_cb.com.OSRdyTbl[TaskId]],
                      gki_calc_stack(TaskId), gki_cb.com.OSStackSize[TaskId]);
        }
    }

#endif
}


/*******************************************************************************
**
** Function         gki_print_buffer_statistics
**
** Description      Called internally by OSS to print the buffer pools statistics
**
** Returns          void
**
*******************************************************************************/
void gki_print_buffer_statistics(FP_PRINT print, int16_t pool)
{
    uint16_t           i;
    BUFFER_HDR_T    *hdr;
    uint16_t           size, act_size, maxbuffs;
    uint32_t           *magic;

    if(pool > GKI_NUM_TOTAL_BUF_POOLS || pool < 0) {
        print("Not a valid Buffer pool\n");
        return;
    }

    size = gki_cb.com.freeq[pool].size;
    maxbuffs = gki_cb.com.freeq[pool].total;
    act_size = size + BUFFER_PADDING_SIZE;
    print("Buffer Pool[%u] size=%u cur_cnt=%u max_cnt=%u  total=%u\n",
          pool, gki_cb.com.freeq[pool].size,
          gki_cb.com.freeq[pool].cur_cnt, gki_cb.com.freeq[pool].max_cnt, gki_cb.com.freeq[pool].total);
    print("      Owner  State    Sanity\n");
    print("----------------------------\n");
    hdr = (BUFFER_HDR_T *)(gki_cb.com.pool_start[pool]);

    for(i = 0; i < maxbuffs; i++) {
        magic = (uint32_t *)((uint8_t *)hdr + BUFFER_HDR_SIZE + size);
        print("%3d: 0x%02x %4d %10s\n", i, hdr->task_id, hdr->status,
              (*magic == MAGIC_NO) ? "OK" : "CORRUPTED");
        hdr          = (BUFFER_HDR_T *)((uint8_t *)hdr + act_size);
    }

    return;
}


/*******************************************************************************
**
** Function         gki_print_used_bufs
**
** Description      Dumps used buffers in the particular pool
**
*******************************************************************************/
GKI_API void gki_print_used_bufs(FP_PRINT print, uint8_t pool_id)
{
    uint8_t        *p_start;
    uint16_t       buf_size;
    uint16_t       num_bufs;
    BUFFER_HDR_T *p_hdr;
    uint16_t       i;
    uint32_t         *magic;
    uint16_t       *p;

    if((pool_id >= GKI_NUM_TOTAL_BUF_POOLS) || (gki_cb.com.pool_start[pool_id] != 0)) {
        print("Not a valid Buffer pool\n");
        return;
    }

    p_start = gki_cb.com.pool_start[pool_id];
    buf_size = gki_cb.com.freeq[pool_id].size + BUFFER_PADDING_SIZE;
    num_bufs = gki_cb.com.freeq[pool_id].total;

    for(i = 0; i < num_bufs; i++, p_start += buf_size) {
        p_hdr = (BUFFER_HDR_T *)p_start;
        magic = (uint32_t *)((uint8_t *)p_hdr + buf_size - sizeof(uint32_t));
        p     = (uint16_t *) p_hdr;

        if(p_hdr->status != BUF_STATUS_FREE) {
            print("%d:0x%x (Q:%d,Task:%s,Stat:%d,%s) %04x %04x %04x %04x %04x %04x %04x %04x\n",
                  i, p_hdr,
                  p_hdr->q_id,
                  GKI_map_taskname(p_hdr->task_id),
                  p_hdr->status,
                  (*magic == MAGIC_NO) ? "OK" : "CORRUPTED",
                  p[0], p[1], p[2], p[3], p[4], p[5], p[6], p[7]);
        }
    }
}


/*******************************************************************************
**
** Function         gki_print_task
**
** Description      This function prints the task states.
**
** Returns          void
**
*******************************************************************************/
void gki_print_task(FP_PRINT print)
{
    uint8_t i;
    print("TID VID TASKNAME STATE WAIT WAITFOR TIMEOUT STACK\n");
    print("-------------------------------------------------\n");

    for(i = 0; i < GKI_MAX_TASKS; i++) {
        if(gki_cb.com.OSRdyTbl[i] != TASK_DEAD) {
            print("%2u  %-8s %-5s %04X    %04X %7u %u/%u Bytes\n",
                  (uint16_t)i,  gki_cb.com.OSTName[i],
                  OSTaskStates[gki_cb.com.OSRdyTbl[i]],
                  gki_cb.com.OSWaitEvt[i], gki_cb.com.OSWaitForEvt[i],
                  gki_cb.com.OSWaitTmr[i], gki_calc_stack(i), gki_cb.com.OSStackSize[i]);
        }
    }
}


/*******************************************************************************
**
** Function         gki_print_exception
**
** Description      This function prints the exception information.
**
** Returns          void
**
*******************************************************************************/
void gki_print_exception(FP_PRINT print)
{
    uint16_t i;
    EXCEPTION_T *pExp;
    print("GKI Exceptions:\n");

    for(i = 0; i < gki_cb.com.ExceptionCnt; i++) {
        pExp =     &gki_cb.com.Exception[i];
        print("%d: Type=%d, Task=%d: %s\n", i,
              (int32_t)pExp->type, (int32_t)pExp->taskid, (int8_t *)pExp->msg);
    }
}


/*****************************************************************************/
void gki_dump(uint8_t *s, uint16_t len, FP_PRINT print)
{
    uint16_t i, j;

    for(i = 0, j = 0; i < len; i++) {
        if(j == 0) {
            print("\n%lX: %02X, ", &s[i], s[i]);
        } else if(j == 7) {
            print("%02X,  ", s[i]);
        } else {
            print("%02X, ", s[i]);
        }

        if(++j == 16) {
            j = 0;
        }
    }

    print("\n");
}

void gki_dump2(uint16_t *s, uint16_t len, FP_PRINT print)
{
    uint16_t i, j;

    for(i = 0, j = 0; i < len; i++) {
        if(j == 0) {
            print("\n%lX: %04X, ", &s[i], s[i]);
        } else {
            print("%04X, ", s[i]);
        }

        if(++j == 8) {
            j = 0;
        }
    }

    print("\n");
}

void gki_dump4(uint32_t *s, uint16_t len, FP_PRINT print)
{
    uint16_t i, j;

    for(i = 0, j = 0; i < len; i++) {
        if(j == 0) {
            print("\n%lX: %08lX, ", &s[i], s[i]);
        } else {
            print("%08lX, ", s[i]);
        }

        if(++j == 4) {
            j = 0;
        }
    }

    print("\n");
}


#endif
