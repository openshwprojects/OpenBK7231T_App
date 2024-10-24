/******************************************************************************

  Copyright (C) 2015 Winner Micro electronics Co., Ltd.

 ******************************************************************************
  File Name     : alg.c
  Version       : Initial Draft
  Author        : Li Limin, lilm@winnermicro.com
  Created       : 2015/3/7
  Last Modified :
  Description   : Application layer gateway, (alg) only for apsta

  History       :
  Date          : 2015/3/7
  Author        : Li Limin, lilm@winnermicro.com
  Modification  : Created file

******************************************************************************/
#include <stdio.h>
#include <string.h>
#include "wm_type_def.h"
#include "wm_timer.h"
#include "tls_common.h"
#include "tls_wireless.h"
#include "lwip/ip.h"
#include "lwip/udp.h"
#include "lwip/icmp.h"
#include "lwip/dns.h"
#include "lwip/priv/tcp_priv.h"
#include "lwip/alg.h"
#include "lwip/timeouts.h"
#include "netif/ethernetif.h"

#if TLS_CONFIG_AP_OPT_FWD
extern u8 *hostapd_get_mac(void);
extern u8 *wpa_supplicant_get_mac(void);
extern u8 *wpa_supplicant_get_bssid(void);
extern struct netif *tls_get_netif(void);
extern struct tls_wif *tls_get_wif_data(void);

//#define NAPT_ALLOC_DEBUG
#ifdef  NAPT_ALLOC_DEBUG
static u16 napt4ic_cnt;
static u16 napt4tcp_cnt;
static u16 napt4udp_cnt;
#endif

//#define NAPT_DEBUG
#ifdef  NAPT_DEBUG
#define NAPT_PRINT printf
#else
#define NAPT_PRINT(...)
#endif

#define IP_PROTO_GRE                 47

#define NAPT_ETH_HDR_LEN             sizeof(struct ethhdr)

#define NAPT_CHKSUM_16BIT_LEN        sizeof(u16)

#define NAPT_TABLE_FOREACH(pos, head)\
         for (pos = head.next; NULL != pos; pos = pos->next)

/* napt tcp/udp */
struct napt_addr_4tu{
    struct napt_addr_4tu *next;
    u16 src_port;
    u16 new_port;
    u8 src_ip;
    u8 time_stamp;
};

/* napt icmp */
struct napt_addr_4ic{
    struct napt_addr_4ic *next;
    u16 src_id;/* icmp id */
    u16 new_id;
    u8 src_ip;
    u8 time_stamp;
};

struct napt_addr_gre{
    u8 src_ip;
    u8 time_stamp;
    bool is_used;
};

struct napt_table_head_4tu{
    struct napt_addr_4tu *next;
#ifdef NAPT_TABLE_LIMIT
    u16 cnt;
#endif
};

struct napt_table_head_4ic{
    struct napt_addr_4ic *next;
#ifdef NAPT_TABLE_LIMIT
    u16 cnt;
#endif
};

/* napt lock */
static struct napt_table_head_4tu napt_table_4tcp;
static struct napt_table_head_4tu napt_table_4udp;
static struct napt_table_head_4ic napt_table_4ic;

//#define NAPT_USE_HW_TIMER

//#define NAPT_TABLE_MUTEX_LOCK
#ifdef  NAPT_TABLE_MUTEX_LOCK
static tls_os_sem_t *napt_table_lock_4tcp;
static tls_os_sem_t *napt_table_lock_4udp;
static tls_os_sem_t *napt_table_lock_4ic;
#ifdef  NAPT_USE_HW_TIMER
static bool napt_check_tcp = FALSE;
static bool napt_check_udp = FALSE;
static bool napt_check_ic  = FALSE;
#endif /* NAPT_USE_HW_TIMER */
#endif /* NAPT_TABLE_MUTEX_LOCK */

/* tcp&udp */
static u16 napt_curr_port;

/* icmp id */
static u16 napt_curr_id;

/* gre for vpn */
static struct napt_addr_gre gre_info;

/*****************************************************************************
 Prototype    : alg_napt_mem_alloc
 Description  : alloc memory
 Input        : u32 size        memory size
 Output       : None
 Return Value : void*    NULL   fail
                        !NULL   success
 ------------------------------------------------------------------------------
 
  History        :
  1.Date         : 2015/3/10
    Author       : Li Limin, lilm@winnermicro.com
    Modification : Created function

*****************************************************************************/
static inline void *alg_napt_mem_alloc(u32 size)
{
    return tls_mem_alloc(size);
}

/*****************************************************************************
 Prototype    : alg_napt_mem_free
 Description  : free memory
 Input        : void *p        memory
 Output       : None
 Return Value : void
 ------------------------------------------------------------------------------
 
  History        :
  1.Date         : 2015/3/10
    Author       : Li Limin, lilm@winnermicro.com
    Modification : Created function

*****************************************************************************/
static inline void alg_napt_mem_free(void *p)
{
    tls_mem_free(p);
    return;
}

/*****************************************************************************
 Prototype    : alg_napt_try_lock
 Description  : try hold lock
 Input        : tls_os_sem_t *lock  point lock
 Output       : None
 Return Value : int  0  success
                    -1  failed
 ------------------------------------------------------------------------------
 
  History        :
  1.Date         : 2019/2/1
    Author       : Li Limin, lilm@winnermicro.com
    Modification : Created function

*****************************************************************************/
static inline int alg_napt_try_lock(tls_os_sem_t *lock)
{
    return (tls_os_sem_acquire(lock, HZ / 10) == TLS_OS_SUCCESS) ? 0 : -1;
}

/*****************************************************************************
 Prototype    : alg_napt_lock
 Description  : hold lock
 Input        : tls_os_sem_t *lock  point lock
 Output       : None
 Return Value : void
 ------------------------------------------------------------------------------
 
  History        :
  1.Date         : 2019/2/1
    Author       : Li Limin, lilm@winnermicro.com
    Modification : Created function

*****************************************************************************/
static inline void alg_napt_lock(tls_os_sem_t *lock)
{
    tls_os_sem_acquire(lock, 0);
    return;
}

/*****************************************************************************
 Prototype    : alg_napt_unlock
 Description  : release lock
 Input        : tls_os_sem_t *lock  point lock
 Output       : None
 Return Value : void
 ------------------------------------------------------------------------------
 
  History        :
  1.Date         : 2019/2/1
    Author       : Li Limin, lilm@winnermicro.com
    Modification : Created function

*****************************************************************************/
static inline void alg_napt_unlock(tls_os_sem_t *lock)
{
    tls_os_sem_release(lock);
}

#ifdef NAPT_TABLE_LIMIT
/*****************************************************************************
 Prototype    : alg_napt_table_is_full
 Description  : the napt table is full
 Input        : void
 Output       : None
 Return Value : bool    true   the napt table is full
                        false  the napt table not is full
 ------------------------------------------------------------------------------
 
  History        :
  1.Date         : 2015/3/10
    Author       : Li Limin, lilm@winnermicro.com
    Modification : Created function

*****************************************************************************/
static inline bool alg_napt_table_is_full(void)
{
    bool is_full = false;

    if ((napt_table_4tcp.cnt + napt_table_4udp.cnt + napt_table_4ic.cnt) >= NAPT_TABLE_SIZE_MAX)
    {
#ifdef NAPT_ALLOC_DEBUG
        printf("@@@ napt batle: limit is reached for tcp/udp.\r\n");
#endif
        NAPT_PRINT("napt batle: limit is reached for tcp/udp.\n");
        is_full = true;
    }

    return is_full;
}
#endif

/*****************************************************************************
 Prototype    : alg_napt_port_alloc
 Description  : alloc napt port
 Input        : void
 Output       : None
 Return Value : u16    0      fail
                       other  success
 ------------------------------------------------------------------------------
 
  History        :
  1.Date         : 2015/3/10
    Author       : Li Limin, lilm@winnermicro.com
    Modification : Created function

*****************************************************************************/
static inline u16 alg_napt_port_alloc(void)
{
    u8_t i;
    u16 cnt = 0;
    struct udp_pcb *udp_pcb;
    struct tcp_pcb *tcp_pcb;
    struct napt_addr_4tu *napt_tcp;
    struct napt_addr_4tu *napt_udp;

again:
    if (napt_curr_port++ == NAPT_LOCAL_PORT_RANGE_END)
    {
        napt_curr_port = NAPT_LOCAL_PORT_RANGE_START;
    }

    /* udp */
    for(udp_pcb = udp_pcbs; udp_pcb != NULL; udp_pcb = udp_pcb->next)
    {
        if (udp_pcb->local_port == napt_curr_port)
        {
            if (++cnt > (NAPT_LOCAL_PORT_RANGE_END - NAPT_LOCAL_PORT_RANGE_START))
            {
                return 0;
            }
            goto again;
        }
    }

    /* tcp */
    for (i = 0; i < NUM_TCP_PCB_LISTS; i++)
    {
        for(tcp_pcb = *tcp_pcb_lists[i]; tcp_pcb != NULL; tcp_pcb = tcp_pcb->next)
        {
            if (tcp_pcb->local_port == napt_curr_port)
            {
                if (++cnt > (NAPT_LOCAL_PORT_RANGE_END - NAPT_LOCAL_PORT_RANGE_START))
                {
                    return 0;
                }
                goto again;
            }
        }
    }

    /* tcp napt */
    NAPT_TABLE_FOREACH(napt_tcp, napt_table_4tcp)
    {
        if (napt_tcp->new_port == napt_curr_port)
        {
            if (++cnt > (NAPT_LOCAL_PORT_RANGE_END - NAPT_LOCAL_PORT_RANGE_START))
            {
                return 0;
            }
            goto again;
        }
    }

    /* udp napt */
    NAPT_TABLE_FOREACH(napt_udp, napt_table_4udp)
    {
        if (napt_udp->new_port == napt_curr_port)
        {
            if (++cnt > (NAPT_LOCAL_PORT_RANGE_END - NAPT_LOCAL_PORT_RANGE_START))
            {
                return 0;
            }
            goto again;
        }
    }

    return napt_curr_port;
}

/*****************************************************************************
 Prototype    : alg_napt_get_by_src_id
 Description  : find item by ip and id
 Input        : u16 id          icmp echo id
                u8  ip          source ip
 Output       : None
 Return Value : struct napt_addr_4ic *      NULL   fail
                                           !NULL   success
 ------------------------------------------------------------------------------
 
  History        :
  1.Date         : 2015/3/10
    Author       : Li Limin, lilm@winnermicro.com
    Modification : Created function

*****************************************************************************/
static inline struct napt_addr_4ic *alg_napt_get_by_src_id(u16 id, u8 ip)
{
    struct napt_addr_4ic *ret = NULL;
    struct napt_addr_4ic *napt;

    NAPT_TABLE_FOREACH(napt, napt_table_4ic)
    {
        if ((id == napt->src_id) && (ip == napt->src_ip))
        {
	       ret = napt;
	       break;
        }
    }

    return ret;
}

/*****************************************************************************
 Prototype    : alg_napt_get_by_dst_id
 Description  : find item by dest id
 Input        : u16 id          icmp echo id
 Output       : None
 Return Value : struct napt_addr_4ic *      NULL   fail
                                           !NULL   success
 ------------------------------------------------------------------------------

  History        :
  1.Date         : 2015/3/10
    Author       : Li Limin, lilm@winnermicro.com
    Modification : Created function

*****************************************************************************/
static inline struct napt_addr_4ic *alg_napt_get_by_dst_id(u16 id)
{
    struct napt_addr_4ic *ret = NULL;
    struct napt_addr_4ic *napt;

    NAPT_TABLE_FOREACH(napt, napt_table_4ic)
    {
        if (id == napt->new_id)
        {
	       ret = napt;
	       break;
        }
    }

    return ret;
}

/*****************************************************************************
 Prototype    : alg_napt_icmp_id_alloc
 Description  : alloc imcp id
 Input        : void
 Output       : None
 Return Value : u16    0      fail
                       other  success
 ------------------------------------------------------------------------------

  History        :
  1.Date         : 2015/3/10
    Author       : Li Limin, lilm@winnermicro.com
    Modification : Created function

*****************************************************************************/
static u16 alg_napt_icmp_id_alloc(void)
{
    u16 cnt = 0;
    struct napt_addr_4ic *napt;

again:
    if (napt_curr_id++ == NAPT_ICMP_ID_RANGE_END)
    {
        napt_curr_id = NAPT_ICMP_ID_RANGE_START;
    }

    NAPT_TABLE_FOREACH(napt, napt_table_4ic)
    {
        if (napt_curr_id == napt->new_id)
        {
            if (++cnt > (NAPT_ICMP_ID_RANGE_END - NAPT_ICMP_ID_RANGE_START))
            {
                return 0;
            }

	       goto again;
        }
    }

    return napt_curr_id;
}

/*****************************************************************************
 Prototype    : alg_napt_table_insert_4ic
 Description  : create napt item
 Input        : u16 id          source id
                u8  ip          source ip
 Output       : None
 Return Value : struct napt_addr_4ic *      NULL   fail
                                           !NULL   success
 Note         : icmp item no limit
 ------------------------------------------------------------------------------
 
  History        :
  1.Date         : 2015/3/10
    Author       : Li Limin, lilm@winnermicro.com
    Modification : Created function

*****************************************************************************/
static inline struct napt_addr_4ic *alg_napt_table_insert_4ic(u16 id, u8 ip)
{
    struct napt_addr_4ic *napt;

#ifdef NAPT_TABLE_LIMIT
    if (true == alg_napt_table_is_full())
    {
        return NULL;
    }
#endif

    napt = alg_napt_mem_alloc(sizeof(struct napt_addr_4ic));
    if (NULL == napt)
    {
        return NULL;
    }

    memset(napt, 0, sizeof(struct napt_addr_4ic));
    napt->src_id = id;

    napt->new_id = alg_napt_icmp_id_alloc();
    if (0 == napt->new_id)
    {
        alg_napt_mem_free(napt);
        return NULL;
    }

    napt->src_ip = ip;
    napt->time_stamp++;

#ifdef NAPT_TABLE_LIMIT
    napt_table_4ic.cnt++;
#endif
    napt->next = napt_table_4ic.next;
    napt_table_4ic.next = napt;
    
#ifdef NAPT_ALLOC_DEBUG
    printf("@@ napt id alloc %hu\r\n", ++napt4ic_cnt);
#endif

    return napt;
}

/*****************************************************************************
 Prototype    : alg_napt_table_update_4ic
 Description  : fresh napt item
 Input        : struct napt_addr_4ic *napt
 Output       : None
 Return Value : void
 ------------------------------------------------------------------------------
 
  History        :
  1.Date         : 2015/3/10
    Author       : Li Limin, lilm@winnermicro.com
    Modification : Created function

*****************************************************************************/
static inline void alg_napt_table_update_4ic(struct napt_addr_4ic *napt)
{
    if (!++napt->time_stamp)
        napt->time_stamp++;

    return;
}

/*****************************************************************************
 Prototype    : alg_napt_get_tcp_port_by_dest
 Description  : find item by port
 Input        : u16 port    dest port
 Output       : None
 Return Value : struct napt_addr_4tu *      NULL   fail
                                           !NULL   success
 ------------------------------------------------------------------------------
 
  History        :
  1.Date         : 2015/3/10
    Author       : Li Limin, lilm@winnermicro.com
    Modification : Created function

*****************************************************************************/
static inline struct napt_addr_4tu *alg_napt_get_tcp_port_by_dest(u16 port)
{
    struct napt_addr_4tu *ret = NULL;
    struct napt_addr_4tu *napt;

    NAPT_TABLE_FOREACH(napt, napt_table_4tcp)
    {
        if (napt->new_port == port)
        {
            ret = napt;
            break;
        }
    }

    return ret;
}

/*****************************************************************************
 Prototype    : alg_napt_get_tcp_port_by_src
 Description  : find item by port and ip
 Input        : u16 port    source port
                u8  ip      source ip
 Output       : None
 Return Value : struct napt_addr_4tu *      NULL   fail
                                           !NULL   success
 ------------------------------------------------------------------------------
 
  History        :
  1.Date         : 2015/3/10
    Author       : Li Limin, lilm@winnermicro.com
    Modification : Created function

*****************************************************************************/
static inline struct napt_addr_4tu *alg_napt_get_tcp_port_by_src(u16 port, u8 ip)
{
    struct napt_addr_4tu *ret = NULL;
    struct napt_addr_4tu *napt;

    NAPT_TABLE_FOREACH(napt, napt_table_4tcp)
    {
        if (port == napt->src_port)
        {
            if (ip == napt->src_ip)
            {
                ret = napt;
                break;
            }
        }
    }

    return ret;
}

/*****************************************************************************
 Prototype    : alg_napt_table_insert_4tcp
 Description  : create napt item
 Input        : u16 src_port    source port
                u8  ip          soutce ip
 Output       : None
 Return Value : struct napt_addr_4tu *      NULL   fail
                                           !NULL   success
 ------------------------------------------------------------------------------
 
  History        :
  1.Date         : 2015/3/10
    Author       : Li Limin, lilm@winnermicro.com
    Modification : Created function

*****************************************************************************/
static inline struct napt_addr_4tu *alg_napt_table_insert_4tcp(u16 src_port, u8 ip)
{
    u16 new_port;
    struct napt_addr_4tu *napt;

#ifdef NAPT_TABLE_LIMIT
    if (true == alg_napt_table_is_full())
    {
        return NULL;
    }
#endif

    napt = alg_napt_mem_alloc(sizeof(struct napt_addr_4tu));
    if (NULL == napt)
    {
        return NULL;
    }

    memset(napt, 0, sizeof(struct napt_addr_4tu));
    napt->src_port = src_port;

    new_port = alg_napt_port_alloc();
    if (0 == new_port)
    {
        alg_napt_mem_free(napt);
        return NULL;
    }

    napt->new_port = htons(new_port);
    napt->src_ip = ip;
    napt->time_stamp++;

#ifdef NAPT_TABLE_LIMIT
    napt_table_4tcp.cnt++;
#endif
    napt->next = napt_table_4tcp.next;
    napt_table_4tcp.next = napt;

#ifdef NAPT_ALLOC_DEBUG
    printf("@@ napt tcp port alloc %hu\r\n", ++napt4tcp_cnt);
#endif

    return napt;
}

/*****************************************************************************
 Prototype    : alg_napt_table_update_4tcp
 Description  : fresh napt item
 Input        : struct napt_addr_4tu *napt  napt item
 Output       : None
 Return Value : void
 ------------------------------------------------------------------------------
 
  History        :
  1.Date         : 2015/3/10
    Author       : Li Limin, lilm@winnermicro.com
    Modification : Created function

*****************************************************************************/
static inline void alg_napt_table_update_4tcp(struct napt_addr_4tu *napt)
{
    if (!++napt->time_stamp)
        napt->time_stamp++;

    return;
}

/*****************************************************************************
 Prototype    : alg_napt_get_udp_port_by_dest
 Description  : find item by port
 Input        : u16 port    dest port
 Output       : None
 Return Value : struct napt_addr_4tu *      NULL   fail
                                           !NULL   success
 ------------------------------------------------------------------------------
 
  History        :
  1.Date         : 2015/3/10
    Author       : Li Limin, lilm@winnermicro.com
    Modification : Created function

*****************************************************************************/
static inline struct napt_addr_4tu *alg_napt_get_udp_port_by_dest(u16 port)
{
    struct napt_addr_4tu *ret = NULL;
    struct napt_addr_4tu *napt;

    NAPT_TABLE_FOREACH(napt, napt_table_4udp)
    {
        if (napt->new_port == port)
        {
            ret = napt;
            break;
        }
    }

    return ret;
}

/*****************************************************************************
 Prototype    : alg_napt_get_udp_port_by_src
 Description  : find item by port and ip
 Input        : u16 port    source port
                u8  ip      source ip
 Output       : None
 Return Value : struct napt_addr_4tu *      NULL   fail
                                           !NULL   success
 ------------------------------------------------------------------------------
 
  History        :
  1.Date         : 2015/3/10
    Author       : Li Limin, lilm@winnermicro.com
    Modification : Created function

*****************************************************************************/
static inline struct napt_addr_4tu *alg_napt_get_udp_port_by_src(u16 port, u8 ip)
{
    struct napt_addr_4tu *ret = NULL;
    struct napt_addr_4tu *napt;

    NAPT_TABLE_FOREACH(napt, napt_table_4udp)
    {
        if (port == napt->src_port)
        {
            if (ip == napt->src_ip)
            {
                ret = napt;
                break;
            }
        }
    }

    return ret;
}

/*****************************************************************************
 Prototype    : alg_napt_table_insert_4udp
 Description  : create new item
 Input        : u16 src_port    source port
                u8  ip          dest   port
 Output       : None
 Return Value : struct napt_addr_4tu *      NULL   fail
                                           !NULL   success
 ------------------------------------------------------------------------------
 
  History        :
  1.Date         : 2015/3/10
    Author       : Li Limin, lilm@winnermicro.com
    Modification : Created function

*****************************************************************************/
static inline struct napt_addr_4tu *alg_napt_table_insert_4udp(u16 src_port, u8 ip)
{
    u16 new_port;
    struct napt_addr_4tu *napt;

#ifdef NAPT_TABLE_LIMIT
    if (true == alg_napt_table_is_full())
    {
        return NULL;
    }
#endif

    napt = alg_napt_mem_alloc(sizeof(struct napt_addr_4tu));
    if (NULL == napt)
    {
        return NULL;
    }

    memset(napt, 0, sizeof(struct napt_addr_4tu));
    napt->src_port = src_port;

    new_port = alg_napt_port_alloc();
    if (0 == new_port)
    {
        alg_napt_mem_free(napt);
        return NULL;
    }

    napt->new_port = htons(new_port);
    napt->src_ip = ip;
    napt->time_stamp++;

#ifdef NAPT_TABLE_LIMIT
    napt_table_4udp.cnt++;
#endif
    napt->next = napt_table_4udp.next;
    napt_table_4udp.next = napt;

#ifdef NAPT_ALLOC_DEBUG
    printf("@@ napt udp port alloc %hu\r\n", ++napt4udp_cnt);
#endif

    return napt;
}

/*****************************************************************************
 Prototype    : alg_napt_table_update_4udp
 Description  : fresh napt item
 Input        : struct napt_addr_4tu *napt
 Output       : None
 Return Value : void
 ------------------------------------------------------------------------------
 
  History        :
  1.Date         : 2015/3/10
    Author       : Li Limin, lilm@winnermicro.com
    Modification : Created function

*****************************************************************************/
static inline void alg_napt_table_update_4udp(struct napt_addr_4tu *napt)
{
    if (!++napt->time_stamp)
        napt->time_stamp++;

    return;
}

/*****************************************************************************
 Prototype    : alg_napt_table_check_4tcp
 Description  : tcp event handle
 Input        : void
 Output       : None
 Return Value : void
 ------------------------------------------------------------------------------
 
  History        :
  1.Date         : 2015/3/10
    Author       : Li Limin, lilm@winnermicro.com
    Modification : Created function

*****************************************************************************/
static void alg_napt_table_check_4tcp(void)
{
    struct napt_addr_4tu *napt4tcp;
    struct napt_addr_4tu *napt4tcp_prev;

    /* tcp */
#ifdef NAPT_TABLE_MUTEX_LOCK
#ifdef NAPT_USE_HW_TIMER
    if (alg_napt_try_lock(napt_table_lock_4tcp))
    {
        //printf("## try tcp\r\n");
        napt_check_tcp = TRUE;
        return;
    }
    napt_check_tcp = FALSE;
#else
    alg_napt_lock(napt_table_lock_4tcp);
#endif
#endif
    for (napt4tcp_prev = napt_table_4tcp.next;\
         NULL != napt4tcp_prev;\
         napt4tcp_prev = napt4tcp_prev->next)
    {
        napt4tcp = napt4tcp_prev->next;
        if (NULL != napt4tcp)
        {
            if (0 == napt4tcp->time_stamp)
            {
#ifdef NAPT_TABLE_LIMIT
                napt_table_4tcp.cnt--;
#endif
                napt4tcp_prev->next = napt4tcp->next;
                napt4tcp->next = NULL;
                alg_napt_mem_free(napt4tcp);
#ifdef NAPT_ALLOC_DEBUG
                printf("@@ napt tcp port free %hu\r\n", --napt4tcp_cnt);
#endif
            }
            else
            {
                napt4tcp->time_stamp = 0;
            }
        }
        
    }
    napt4tcp = napt_table_4tcp.next;
    if (NULL != napt4tcp)
    {
        if (0 == napt4tcp->time_stamp)
        {
#ifdef NAPT_TABLE_LIMIT
            napt_table_4tcp.cnt--;
#endif
            napt_table_4tcp.next = napt4tcp->next;
            napt4tcp->next = NULL;
            alg_napt_mem_free(napt4tcp);
#ifdef NAPT_ALLOC_DEBUG
            printf("@@ napt tcp port free %hu\r\n", --napt4tcp_cnt);
#endif
        }
        else
        {
            napt4tcp->time_stamp = 0;
        }
    }
#ifdef NAPT_TABLE_MUTEX_LOCK
    alg_napt_unlock(napt_table_lock_4tcp);
#endif
    return;
}

/*****************************************************************************
 Prototype    : alg_napt_table_check_4udp
 Description  : udp event handle
 Input        : void
 Output       : None
 Return Value : void
 ------------------------------------------------------------------------------
 
  History        :
  1.Date         : 2015/3/10
    Author       : Li Limin, lilm@winnermicro.com
    Modification : Created function

*****************************************************************************/
static void alg_napt_table_check_4udp(void)
{
    struct napt_addr_4tu *napt4udp;
    struct napt_addr_4tu *napt4udp_prev;

    /* udp */
#ifdef NAPT_TABLE_MUTEX_LOCK
#ifdef NAPT_USE_HW_TIMER
    if (alg_napt_try_lock(napt_table_lock_4udp))
    {
        //printf("## try udp\r\n");
        napt_check_udp = TRUE;
        return;
    }
    napt_check_udp = FALSE;
#else
    alg_napt_lock(napt_table_lock_4udp);
#endif
#endif
    for (napt4udp_prev = napt_table_4udp.next;\
         NULL != napt4udp_prev;\
         napt4udp_prev = napt4udp_prev->next)
    {
        napt4udp = napt4udp_prev->next;
        if (NULL != napt4udp)
        {
            if (0 == napt4udp->time_stamp)
            {
#ifdef NAPT_TABLE_LIMIT
                napt_table_4udp.cnt--;
#endif
                napt4udp_prev->next = napt4udp->next;
                napt4udp->next = NULL;
                alg_napt_mem_free(napt4udp);
#ifdef NAPT_ALLOC_DEBUG
                printf("@@ napt udp port free %hu\r\n", --napt4udp_cnt);
#endif
            }
            else
            {
                napt4udp->time_stamp = 0;
            }
        }
        
    }
    napt4udp = napt_table_4udp.next;
    if (NULL != napt4udp)
    {
        if (0 == napt4udp->time_stamp)
        {
#ifdef NAPT_TABLE_LIMIT
            napt_table_4udp.cnt--;
#endif
            napt_table_4udp.next = napt4udp->next;
            napt4udp->next = NULL;
            alg_napt_mem_free(napt4udp);
#ifdef NAPT_ALLOC_DEBUG
            printf("@@ napt udp port free %hu\r\n", --napt4udp_cnt);
#endif
        }
        else
        {
            napt4udp->time_stamp = 0;
        }
    }
#ifdef NAPT_TABLE_MUTEX_LOCK
    alg_napt_unlock(napt_table_lock_4udp);
#endif
    return;
}

/*****************************************************************************
 Prototype    : alg_napt_table_check_4ic
 Description  : icmp event handle
 Input        : void
 Output       : None
 Return Value : void
 ------------------------------------------------------------------------------
 
  History        :
  1.Date         : 2015/3/10
    Author       : Li Limin, lilm@winnermicro.com
    Modification : Created function

*****************************************************************************/
static void alg_napt_table_check_4ic(void)
{
    struct napt_addr_4ic *napt4ic;
    struct napt_addr_4ic *napt4ic_prev;

    /* icmp */
#ifdef NAPT_TABLE_MUTEX_LOCK
#ifdef NAPT_USE_HW_TIMER
    if (alg_napt_try_lock(napt_table_lock_4ic))
    {
        //printf("## try ic\r\n");
        napt_check_ic = TRUE;
        return;
    }
    napt_check_ic = FALSE;
#else
    alg_napt_lock(napt_table_lock_4ic);
#endif
#endif
    /* skip the first item */
    for (napt4ic_prev = napt_table_4ic.next;\
         NULL != napt4ic_prev;\
         napt4ic_prev = napt4ic_prev->next)
    {
        napt4ic = napt4ic_prev->next;
        if (NULL != napt4ic)
        {
            if (0 == napt4ic->time_stamp)
            {
#ifdef NAPT_TABLE_LIMIT
                napt_table_4ic.cnt--;
#endif
                napt4ic_prev->next = napt4ic->next;
                napt4ic->next = NULL;
                alg_napt_mem_free(napt4ic);
#ifdef NAPT_ALLOC_DEBUG
                printf("@@ napt id free %hu\r\n", --napt4ic_cnt);
#endif
            }
            else
            {
                napt4ic->time_stamp = 0;
            }
        }
        
    }
    /* check the first item */
    napt4ic = napt_table_4ic.next;
    if (NULL != napt4ic)
    {
        if (0 == napt4ic->time_stamp)
        {
#ifdef NAPT_TABLE_LIMIT
            napt_table_4ic.cnt--;
#endif
            napt_table_4ic.next = napt4ic->next;
            napt4ic->next = NULL;
            alg_napt_mem_free(napt4ic);
#ifdef NAPT_ALLOC_DEBUG
            printf("@@ napt id free %hu\r\n", --napt4ic_cnt);
#endif
        }
        else
        {
            napt4ic->time_stamp = 0;
        }
    }
#ifdef NAPT_TABLE_MUTEX_LOCK
    alg_napt_unlock(napt_table_lock_4ic);
#endif
    return;
}

/*****************************************************************************
 Prototype    : alg_napt_table_check_4gre
 Description  : vpn event handle
 Input        : void
 Output       : None
 Return Value : void
 ------------------------------------------------------------------------------
 
  History        :
  1.Date         : 2015/3/10
    Author       : Li Limin, lilm@winnermicro.com
    Modification : Created function

*****************************************************************************/
static void alg_napt_table_check_4gre(void)
{
    if (0 == gre_info.time_stamp)
    {
        gre_info.is_used = false;
        //gre_info.src_ip = 0;
    }
    else
    {
        gre_info.time_stamp = 0;
    }

    return;
}

/*****************************************************************************
 Prototype    : alg_napt_table_check
 Description  : napt table age check
 Input        : u32 type  event type
 Output       : None
 Return Value : void
 ------------------------------------------------------------------------------
 
  History        :
  1.Date         : 2015/3/10
    Author       : Li Limin, lilm@winnermicro.com
    Modification : Created function

*****************************************************************************/
static void alg_napt_table_check(void *arg)
{
    alg_napt_table_check_4tcp();
    alg_napt_table_check_4udp();
    alg_napt_table_check_4ic();
    alg_napt_table_check_4gre();

    sys_timeout((u32)arg, alg_napt_table_check, arg);

    return;
}

#ifdef NAPT_USE_HW_TIMER
static void alg_napt_table_check_try(void)
{
    if (napt_check_tcp)
        alg_napt_table_check_4tcp();

    if (napt_check_udp)
        alg_napt_table_check_4udp();

    if (napt_check_ic)
        alg_napt_table_check_4ic();

    return;
}
#endif

/*****************************************************************************
 Prototype    : alg_hdr_16bitsum
 Description  : check sum
 Input        : u16 *buff  packet
                u16 len    packet length
 Output       : None
 Return Value : u32     sum
 ------------------------------------------------------------------------------
 
  History        :
  1.Date         : 2015/3/10
    Author       : Li Limin, lilm@winnermicro.com
    Modification : Created function

*****************************************************************************/
static inline u32 alg_hdr_16bitsum(const u16 *buff, u16 len)
{
    u32 sum = 0;

    u16 *pos = (u16 *)buff;
    u16 remainder_size = len;

    while (remainder_size > 1)
    {
        sum += *pos ++;
        remainder_size -= NAPT_CHKSUM_16BIT_LEN;
    }

    if (remainder_size > 0)
    {
        sum += *(u8*)pos;
    }

    return sum;
}

/*****************************************************************************
 Prototype    : alg_iphdr_chksum
 Description  : check sum
 Input        : u16 *buff  ip header
                u16 len    ip header length
 Output       : None
 Return Value : u16 chksum
 ------------------------------------------------------------------------------
 
  History        :
  1.Date         : 2015/3/10
    Author       : Li Limin, lilm@winnermicro.com
    Modification : Created function

*****************************************************************************/
static inline u16 alg_iphdr_chksum(const u16 *buff, u16 len)
{
    u32 sum = alg_hdr_16bitsum(buff, len);

    sum = (sum >> 16) + (sum & 0xFFFF);
    sum += (sum >> 16);

    return (u16)(~sum);
}

/*****************************************************************************
 Prototype    : alg_tcpudphdr_chksum
 Description  : check sum
 Input        : u32 src_addr  source ip
                u32 dst_addr  dest ip
                u8 proto      ip packet protocol
                u16 *buff     payload
                u16 len       payload length (without ip header)
 Output       : None
 Return Value : u16  chksum
 ------------------------------------------------------------------------------
 
  History        :
  1.Date         : 2015/3/10
    Author       : Li Limin, lilm@winnermicro.com
    Modification : Created function

*****************************************************************************/
static inline u16 alg_tcpudphdr_chksum(u32 src_addr, u32 dst_addr, u8 proto,
                                       const u16 *buff, u16 len)
{
    u32 sum = 0;

    sum += (src_addr & 0xffffUL);
    sum += ((src_addr >> 16) & 0xffffUL);
    sum += (dst_addr & 0xffffUL);
    sum += ((dst_addr >> 16) & 0xffffUL);
    sum += (u32)htons((u16)proto);
    sum += (u32)htons(len);

    sum += alg_hdr_16bitsum(buff, len);

    sum = (sum >> 16) + (sum & 0xFFFF);
    sum += (sum >> 16);

    return (u16)(~sum);
}

/*****************************************************************************
 Prototype    : alg_output
 Description  : wifi mac layer forward
 Input        : u8 *ehdr
                u16 eth_len
 Output       : None
 Return Value : int      0   success
                        -1   fail
 ------------------------------------------------------------------------------
 
  History        :
  1.Date         : 2015/3/10
    Author       : Li Limin, lilm@winnermicro.com
    Modification : Created function

*****************************************************************************/
static inline int alg_output(u8 *ehdr, u16 eth_len)
{
    int err;
    struct tls_wif *wif = tls_get_wif_data();
    err = tls_wl_if_xmit(wif, ehdr, eth_len, FALSE, FALSE);
    return err;
}

/*****************************************************************************
 Prototype    : alg_output2
 Description  : send by lwip
 Input        : struct netif *netif
                struct ip_hdr *ip_hdr
 Output       : None
 Return Value : int      0   success
                        -1   fail
 ------------------------------------------------------------------------------
 
  History        :
  1.Date         : 2015/3/10
    Author       : Li Limin, lilm@winnermicro.com
    Modification : Created function

*****************************************************************************/
static inline int alg_output2(struct netif *netif, const struct ip_hdr *ip_hdr)
{
    int err;
    u16 len;
    ip_addr_t ipaddr;
    struct pbuf *pBuf;

    len = ntohs(ip_hdr->_len);
	pBuf = pbuf_alloc(PBUF_LINK, len, PBUF_RAM);
	if(pBuf == NULL)
	{
		return -1;
	}

    pbuf_take(pBuf, (const void *)ip_hdr, len);

    memset(&ipaddr, 0, sizeof(ip_addr_t));
    ip_2_ip4(&ipaddr)->addr = ip_hdr->dest.addr;
    
	err = netif->ipfwd_output(pBuf, netif, ipaddr);
	if (0 != err)
	{
        pbuf_free(pBuf);
	}

    return err;
}

/*****************************************************************************
 Prototype    : alg_deliver2lwip
 Description  : deliver to lwip
 Input        : u8 *bssid
                u8 *ehdr
                u16 eth_len
 Output       : None
 Return Value : int      0   success
                        -1   fail
 ------------------------------------------------------------------------------
 
  History        :
  1.Date         : 2015/3/10
    Author       : Li Limin, lilm@winnermicro.com
    Modification : Created function

*****************************************************************************/
static inline int alg_deliver2lwip(const u8 *bssid, u8 *ehdr, u16 eth_len)
{
    int err;

    err = ethernetif_input(bssid, ehdr, eth_len);

    return err;
}

/*****************************************************************************
 Prototype    : alg_icmp_proc
 Description  : icmp packet input
 Input        : u8 *bssid
                struct ip_hdr *ip_hdr
                u8 *ehdr
                u16 eth_len
 Output       : None
 Return Value : int      0   success
                        -1   fail
 ------------------------------------------------------------------------------
 
  History        :
  1.Date         : 2015/3/10
    Author       : Li Limin, lilm@winnermicro.com
    Modification : Created function

  Note:
     --------     ap..      -----------     sta..     ---------
     |  AP  |---------------|  APSTA  |---------------|  STA  |
     --------               -----------               ---------
*****************************************************************************/
static int alg_icmp_proc(const u8 *bssid,
                         struct ip_hdr *ip_hdr,
                         u8 *ehdr, u16 eth_len)
{
    int err;
    struct napt_addr_4ic *napt;
    struct icmp_echo_hdr *icmp_hdr;
    struct netif *net_if = tls_get_netif();
    u8 *mac = wpa_supplicant_get_mac();
    u8 *mac2 = hostapd_get_mac();
    u8 iphdr_len;

    iphdr_len = (ip_hdr->_v_hl & 0x0F) * 4;
    icmp_hdr = (struct icmp_echo_hdr *)((u8 *)ip_hdr + iphdr_len);

    if (0 == compare_ether_addr(bssid, mac2))
    {
        if ((ip_hdr->dest.addr == ip_addr_get_ip4_u32(&net_if->next->ip_addr))||
		    (ip_hdr->dest.addr == ip_addr_get_ip4_u32(&net_if->ip_addr)))
        {
            err = alg_deliver2lwip(bssid, ehdr, eth_len);
            return err;
        }

#ifdef NAPT_TABLE_MUTEX_LOCK
        alg_napt_lock(napt_table_lock_4ic);
#endif
        napt = alg_napt_get_by_src_id(icmp_hdr->id, ip_hdr->src.addr >> 24);
        if (NULL == napt)
        {
            napt = alg_napt_table_insert_4ic(icmp_hdr->id, ip_hdr->src.addr >> 24);
            if (NULL == napt)
            {
#ifdef NAPT_TABLE_MUTEX_LOCK
                alg_napt_unlock(napt_table_lock_4ic);
#endif
                return -1;
            }
        }
        else
        {
            alg_napt_table_update_4ic(napt);
        }

        icmp_hdr->id = napt->new_id;

#ifdef NAPT_TABLE_MUTEX_LOCK
        alg_napt_unlock(napt_table_lock_4ic);
#endif

        icmp_hdr->chksum = 0;
        icmp_hdr->chksum = alg_iphdr_chksum((u16 *)icmp_hdr, ntohs(ip_hdr->_len) - iphdr_len);

        ip_hdr->src.addr = ip_addr_get_ip4_u32(&net_if->ip_addr);
        ip_hdr->_chksum = 0;
        ip_hdr->_chksum = alg_iphdr_chksum((u16 *)ip_hdr, iphdr_len);

        if ((ip_addr_get_ip4_u32(&net_if->ip_addr) & 0xFFFFFF) == (ip_hdr->dest.addr & 0xFFFFFF))
        {
            err = alg_output2(net_if, ip_hdr);
        }
        else /* to the internet */
        {
            memcpy(ehdr, wpa_supplicant_get_bssid(), ETH_ALEN);
            memcpy(ehdr + ETH_ALEN, mac, ETH_ALEN);
            /* forward to ap...*/
            err = alg_output(ehdr, eth_len);
        }
    }
    else
    {
#ifdef NAPT_TABLE_MUTEX_LOCK
        alg_napt_lock(napt_table_lock_4ic);
#endif
        napt = alg_napt_get_by_dst_id(icmp_hdr->id);
        if (NULL != napt)
        {
            //alg_napt_table_update_4ic(napt);

            icmp_hdr->id = napt->src_id;
            icmp_hdr->chksum = 0;
            icmp_hdr->chksum = alg_iphdr_chksum((u16 *)icmp_hdr, ntohs(ip_hdr->_len) - iphdr_len);

            ip_hdr->dest.addr = ((napt->src_ip) << 24) | (ip_addr_get_ip4_u32(&net_if->next->ip_addr) & 0x00ffffff);

#ifdef NAPT_TABLE_MUTEX_LOCK
            alg_napt_unlock(napt_table_lock_4ic);
#endif

            ip_hdr->_chksum = 0;
            ip_hdr->_chksum = alg_iphdr_chksum((u16 *)ip_hdr, iphdr_len);

            memcpy(ehdr, tls_dhcps_getmac((ip_addr_t *)&ip_hdr->dest.addr), ETH_ALEN);
            memcpy(ehdr + ETH_ALEN, mac2, ETH_ALEN);
            err = alg_output(ehdr, eth_len);
        }
        else
        {
#ifdef NAPT_TABLE_MUTEX_LOCK
            alg_napt_unlock(napt_table_lock_4ic);
#endif

            err = alg_deliver2lwip(bssid, ehdr, eth_len);
        }
    }

    return err;
}

/*****************************************************************************
 Prototype    : alg_tcp_proc
 Description  : tcp packet input
 Input        : u8 *bssid
                struct ip_hdr *ip_hdr
                u8 *ehdr
                u16 eth_len
 Output       : None
 Return Value : int      0   success
                        -1   fail
 ------------------------------------------------------------------------------
 
  History        :
  1.Date         : 2015/3/10
    Author       : Li Limin, lilm@winnermicro.com
    Modification : Created function

  Note:
     --------     ap..      -----------     sta..     ---------
     |  AP  |---------------|  APSTA  |---------------|  STA  |
     --------               -----------               ---------
*****************************************************************************/
static int alg_tcp_proc(const u8 *bssid,
                        struct ip_hdr *ip_hdr,
                        u8 *ehdr, u16 eth_len)
{
    int err;
    u8 src_ip;
    struct napt_addr_4tu *napt;
    struct tcp_hdr *tcp_hdr;
    struct netif *net_if = tls_get_netif();
    u8 *mac = wpa_supplicant_get_mac();
    u8 *mac2 = hostapd_get_mac();
    u8 iphdr_len;

    iphdr_len = (ip_hdr->_v_hl & 0x0F) * 4;
    tcp_hdr = (struct tcp_hdr *)((u8 *)ip_hdr + iphdr_len);

    if (0 == compare_ether_addr(bssid, mac2))
    {
        if ((ip_hdr->dest.addr == ip_addr_get_ip4_u32(&net_if->next->ip_addr)) ||
		     (ip_hdr->dest.addr == ip_addr_get_ip4_u32(&net_if->ip_addr)))
        {
            err = alg_deliver2lwip(bssid, ehdr, eth_len);
            return err;
        }

#ifdef NAPT_TABLE_MUTEX_LOCK
        alg_napt_lock(napt_table_lock_4tcp);
#endif
        src_ip = ip_hdr->src.addr >> 24;
        napt = alg_napt_get_tcp_port_by_src(tcp_hdr->src, src_ip);
        if (NULL == napt)
        {
            napt = alg_napt_table_insert_4tcp(tcp_hdr->src, src_ip);
            if (NULL == napt)
            {
#ifdef NAPT_TABLE_MUTEX_LOCK
                alg_napt_unlock(napt_table_lock_4tcp);
#endif
                return -1;
            }
        }
        else
        {
            alg_napt_table_update_4tcp(napt);
        }

        tcp_hdr->src = napt->new_port;
#ifdef NAPT_TABLE_MUTEX_LOCK
        alg_napt_unlock(napt_table_lock_4tcp);
#endif

        ip_hdr->src.addr = ip_addr_get_ip4_u32(&net_if->ip_addr);
        ip_hdr->_chksum = 0;
        ip_hdr->_chksum = alg_iphdr_chksum((u16 *)ip_hdr, iphdr_len);

        tcp_hdr->chksum = 0;
        tcp_hdr->chksum = alg_tcpudphdr_chksum(ip_hdr->src.addr,
                                               ip_hdr->dest.addr,
                                               IP_PROTO_TCP,
                                               (u16 *)tcp_hdr,
                                               ntohs(ip_hdr->_len) - iphdr_len);

        if ((ip_addr_get_ip4_u32(&net_if->ip_addr) & 0xFFFFFF) == (ip_hdr->dest.addr & 0xFFFFFF))
        {
            err = alg_output2(net_if, ip_hdr);
        }
        else /* to the internet */
        {
            memcpy(ehdr, wpa_supplicant_get_bssid(), ETH_ALEN);
            memcpy(ehdr + ETH_ALEN, mac, ETH_ALEN);
            /* forward to ap...*/
            err = alg_output(ehdr, eth_len);
        }
    }
    /* from ap... */
    else
    {
#ifdef NAPT_TABLE_MUTEX_LOCK
        alg_napt_lock(napt_table_lock_4tcp);
#endif
        napt = alg_napt_get_tcp_port_by_dest(tcp_hdr->dest);
        /* forward to sta... */
        if (NULL != napt)
        {
            //alg_napt_table_update_4tcp(napt);

            ip_hdr->dest.addr = (napt->src_ip << 24) | (ip_addr_get_ip4_u32(&net_if->next->ip_addr) & 0x00ffffff);
            ip_hdr->_chksum = 0;
            ip_hdr->_chksum = alg_iphdr_chksum((u16 *)ip_hdr, iphdr_len);

            tcp_hdr->dest = napt->src_port;

#ifdef NAPT_TABLE_MUTEX_LOCK
            alg_napt_unlock(napt_table_lock_4tcp);
#endif

            tcp_hdr->chksum = 0;
            tcp_hdr->chksum = alg_tcpudphdr_chksum(ip_hdr->src.addr,
                                                   ip_hdr->dest.addr,
                                                   IP_PROTO_TCP,
                                                   (u16 *)tcp_hdr,
                                                   ntohs(ip_hdr->_len) - iphdr_len);

            memcpy(ehdr, tls_dhcps_getmac((ip_addr_t *)&ip_hdr->dest.addr), ETH_ALEN);
            memcpy(ehdr + ETH_ALEN, mac2, ETH_ALEN);
            err = alg_output(ehdr, eth_len);
        }
        /* deliver to default gateway */
        else
        {
#ifdef NAPT_TABLE_MUTEX_LOCK
            alg_napt_unlock(napt_table_lock_4tcp);
#endif

            err = alg_deliver2lwip(bssid, ehdr, eth_len);
        }
    }

    return err;
}

/*****************************************************************************
 Prototype    : alg_udp_proc
 Description  : udp packet input
 Input        : u8 *bssid
                struct ip_hdr *ip_hdr
                u8 *ehdr
                u16 eth_len
 Output       : None
 Return Value : int      0   success
                        -1   fail
 ------------------------------------------------------------------------------
 
  History        :
  1.Date         : 2015/3/10
    Author       : Li Limin, lilm@winnermicro.com
    Modification : Created function

  Note:
     --------     ap...     -----------     sta...    ---------
     |  AP  |---------------|  APSTA  |---------------|  STA  |
     --------               -----------               ---------
*****************************************************************************/
static int alg_udp_proc(const u8 *bssid,
                        struct ip_hdr *ip_hdr,
                        u8 *ehdr, u16 eth_len)
{
    int err = 0;
    u8 src_ip;
    struct napt_addr_4tu *napt;
    struct udp_hdr *udp_hdr;
    struct netif *net_if = tls_get_netif();
    u8 *mac = wpa_supplicant_get_mac();
    u8 *mac2 = hostapd_get_mac();
    u8 iphdr_len;
    int is_dns = 0;
    ip_addr_t *dns;

    iphdr_len = (ip_hdr->_v_hl & 0x0F) * 4;
    udp_hdr = (struct udp_hdr *)((u8 *)ip_hdr + iphdr_len);

    /* from sta... */
    if (0 == compare_ether_addr(bssid, mac2))
    {
        /* deliver to alg gateway */
        if ((ip_hdr->dest.addr == ip_addr_get_ip4_u32(&net_if->next->ip_addr))||
		    (ip_hdr->dest.addr == ip_addr_get_ip4_u32(&net_if->ip_addr)))
        {
            err = alg_deliver2lwip(bssid, ehdr, eth_len);
            if ((53 == ntohs(udp_hdr->dest)) && netif_is_up(net_if))/* dns */
            {
                is_dns = 1;
            }
            else
            {
            return err;
            }
        }

        /* create/update napt item */
#ifdef NAPT_TABLE_MUTEX_LOCK
        alg_napt_lock(napt_table_lock_4udp);
#endif
        src_ip = ip_hdr->src.addr >> 24;
        napt = alg_napt_get_udp_port_by_src(udp_hdr->src, src_ip);
        if (NULL == napt)
        {
            napt = alg_napt_table_insert_4udp(udp_hdr->src, src_ip);
            if (NULL == napt)
            {
#ifdef NAPT_TABLE_MUTEX_LOCK
                alg_napt_unlock(napt_table_lock_4udp);
#endif
                return -1;
            }
        }
        else
        {
            alg_napt_table_update_4udp(napt);
        }

        udp_hdr->src = napt->new_port;

#ifdef NAPT_TABLE_MUTEX_LOCK
        alg_napt_unlock(napt_table_lock_4udp);
#endif

redo:
        if (is_dns)
        {
            dns = (ip_addr_t*)dns_getserver(is_dns - 1);
            if ((2 == is_dns) && ip_addr_isany(dns))
                goto end;
            ip_hdr->dest.addr = dns->addr;
        }
        
        ip_hdr->src.addr = ip_addr_get_ip4_u32(&net_if->ip_addr);
        ip_hdr->_chksum = 0;
        ip_hdr->_chksum = alg_iphdr_chksum((u16 *)ip_hdr, iphdr_len);

        if (0 != udp_hdr->chksum)
        {
            udp_hdr->chksum = 0;
            udp_hdr->chksum = alg_tcpudphdr_chksum(ip_hdr->src.addr,
                                                   ip_hdr->dest.addr,
                                                   IP_PROTO_UDP,
                                                   (u16 *)udp_hdr,
                                                   ntohs(ip_hdr->_len) - iphdr_len);
        }

        if ((ip_addr_get_ip4_u32(&net_if->ip_addr) & 0xFFFFFF) == (ip_hdr->dest.addr & 0xFFFFFF))
        {
            err = alg_output2(net_if, ip_hdr);
        }
        else /* to the internet */
        {
            memcpy(ehdr, wpa_supplicant_get_bssid(), ETH_ALEN);
            memcpy(ehdr + ETH_ALEN, mac, ETH_ALEN);
            /* forward to ap...*/
            err = alg_output(ehdr, eth_len);
        }

        if (1 == is_dns)
        {
            is_dns = 2;
            goto redo;
        }
    }
    /* form ap... */
    else
    {
#ifdef NAPT_TABLE_MUTEX_LOCK
        alg_napt_lock(napt_table_lock_4udp);
#endif
        napt = alg_napt_get_udp_port_by_dest(udp_hdr->dest);
        /* forward to sta... */
        if (NULL != napt)
        {
            alg_napt_table_update_4udp(napt);

            ip_hdr->dest.addr = (napt->src_ip << 24) | (ip_addr_get_ip4_u32(&net_if->next->ip_addr) & 0x00ffffff);

            udp_hdr->dest = napt->src_port;

#ifdef NAPT_TABLE_MUTEX_LOCK
            alg_napt_unlock(napt_table_lock_4udp);
#endif

            if (53 == ntohs(udp_hdr->src))
            {
                ip_hdr->src.addr = ip_addr_get_ip4_u32(&net_if->next->ip_addr);
            }

            ip_hdr->_chksum = 0;
            ip_hdr->_chksum = alg_iphdr_chksum((u16 *)ip_hdr, iphdr_len);

            if (0 != udp_hdr->chksum)
            {
                udp_hdr->chksum = 0;
                udp_hdr->chksum = alg_tcpudphdr_chksum(ip_hdr->src.addr,
                                                       ip_hdr->dest.addr,
                                                       IP_PROTO_UDP,
                                                       (u16 *)udp_hdr,
                                                       ntohs(ip_hdr->_len) - iphdr_len);
            }

            memcpy(ehdr, tls_dhcps_getmac((ip_addr_t *)&ip_hdr->dest.addr), ETH_ALEN);
            memcpy(ehdr + ETH_ALEN, mac2, ETH_ALEN);
            err = alg_output(ehdr, eth_len);
        }
        /* deliver to default gateway */
        else
        {
#ifdef NAPT_TABLE_MUTEX_LOCK
            alg_napt_unlock(napt_table_lock_4udp);
#endif

            err = alg_deliver2lwip(bssid, ehdr, eth_len);
        }
    }

end:
    return err;
}

/*****************************************************************************
 Prototype    : alg_gre_proc
 Description  : vpn input
 Input        : u8 *bssid
                struct ip_hdr *ip_hdr
                u8 *ehdr
                u16 eth_len
 Output       : None
 Return Value : int      0   success
                        -1   fail
 ------------------------------------------------------------------------------
 
  History        :
  1.Date         : 2015/3/10
    Author       : Li Limin, lilm@winnermicro.com
    Modification : Created function

  Note:
     --------     ap...     -----------      sta...   ---------
     |  AP  |---------------|  APSTA  |---------------|  STA  |
     --------               -----------               ---------
*****************************************************************************/
static int alg_gre_proc(const u8 *bssid, struct ip_hdr *ip_hdr, u8 *ehdr, u16 eth_len)
{
    int err;
    u8 src_ip;
    u8 iphdr_len;
    struct netif *net_if = tls_get_netif();
    u8 *mac = wpa_supplicant_get_mac();
    u8 *mac2 = hostapd_get_mac();

    iphdr_len = (ip_hdr->_v_hl & 0x0F) * 4;

    /* from sta... */
    if (0 == compare_ether_addr(bssid, mac2))
    {
        src_ip = ip_hdr->src.addr >> 24;

        if (src_ip == gre_info.src_ip)    /* current vpn */
        {
            gre_info.time_stamp++;
        }
        else if (true == gre_info.is_used)/* vpn used */
        {
            return -1;
        }
        else                              /* new vpn */
        {
            gre_info.is_used = true;
            gre_info.src_ip = src_ip;
            if (!++gre_info.time_stamp)
                gre_info.time_stamp++;
        }

        ip_hdr->src.addr = ip_addr_get_ip4_u32(&net_if->ip_addr);
        ip_hdr->_chksum = 0;
        ip_hdr->_chksum = alg_iphdr_chksum((u16 *)ip_hdr, iphdr_len);

        if ((ip_addr_get_ip4_u32(&net_if->ip_addr) & 0xFFFFFF) == (ip_hdr->dest.addr & 0xFFFFFF))
        {
            err = alg_output2(net_if, ip_hdr);
        }
        else /* to the internet */
        {
            memcpy(ehdr, wpa_supplicant_get_bssid(), ETH_ALEN);
            memcpy(ehdr + ETH_ALEN, mac, ETH_ALEN);
            /* forward to ap...*/
            err = alg_output(ehdr, eth_len);
        }
    }
    /* from ap... */
    else
    {
        ip_hdr->dest.addr = (gre_info.src_ip << 24) | (ip_addr_get_ip4_u32(&net_if->next->ip_addr) & 0x00ffffff);
        ip_hdr->_chksum = 0;
        ip_hdr->_chksum = alg_iphdr_chksum((u16 *)ip_hdr, iphdr_len);

        memcpy(ehdr, tls_dhcps_getmac((ip_addr_t *)&ip_hdr->dest.addr), ETH_ALEN);
        memcpy(ehdr + ETH_ALEN, mac2, ETH_ALEN);
        err = alg_output(ehdr, eth_len);
    }

    return err;
}

/*****************************************************************************
 Prototype    : alg_input
 Description  : ip packet input
 Input        : u8 *bssid            BSSID
                u8 *pkt_body         packet
                u32 pkt_len          length
 Output       : None
 Return Value : int      0  success
                        -1  fail
 ------------------------------------------------------------------------------
 
  History        :
  1.Date         : 2015/3/10
    Author       : Li Limin, lilm@winnermicro.com
    Modification : Created function

*****************************************************************************/
int alg_input(const u8 *bssid, u8 *pkt_body, u32 pkt_len)
{
    int err;
    struct ip_hdr *ip_hdr;

#ifdef NAPT_USE_HW_TIMER
    alg_napt_table_check_try();
#endif

    ip_hdr = (struct ip_hdr *)(pkt_body + NAPT_ETH_HDR_LEN);
    switch(ip_hdr->_proto)
    {
        case IP_PROTO_ICMP:
        {
            err = alg_icmp_proc(bssid, ip_hdr, pkt_body, (u16)pkt_len);
            break;
        }
        case IP_PROTO_TCP:
        {
            err = alg_tcp_proc(bssid, ip_hdr, pkt_body, (u16)pkt_len);
            break;
        }
        case IP_PROTO_UDP:
        {
            err = alg_udp_proc(bssid, ip_hdr, pkt_body, (u16)pkt_len);
            break;
        }
        case IP_PROTO_GRE:/* vpn */
        {
            err = alg_gre_proc(bssid, ip_hdr, pkt_body, (u16)pkt_len);
        }
        default:
        {
            err = -1;
            break;
        }
    }

    return err;
}

/*****************************************************************************
 Prototype    : alg_napt_port_is_used
 Description  : port is used
 Input        : u16 port        port number
 Output       : None
 Return Value : bool    true    used
                        false   unused
 ------------------------------------------------------------------------------
 
  History        :
  1.Date         : 2015/3/10
    Author       : Li Limin, lilm@winnermicro.com
    Modification : Created function

*****************************************************************************/
bool alg_napt_port_is_used(u16 port)
{
    bool is_used = false;
    struct napt_addr_4tu *napt_tcp;
    struct napt_addr_4tu *napt_udp;

#ifdef NAPT_TABLE_MUTEX_LOCK
    alg_napt_lock(napt_table_lock_4tcp);
#endif
    NAPT_TABLE_FOREACH(napt_tcp, napt_table_4tcp)
    {
        if (port == napt_tcp->new_port)
        {
            is_used = true;
            break;
        }
    }
#ifdef NAPT_TABLE_MUTEX_LOCK
    alg_napt_unlock(napt_table_lock_4tcp);
#endif

    if (true != is_used)
    {
#ifdef NAPT_TABLE_MUTEX_LOCK
        alg_napt_lock(napt_table_lock_4udp);
#endif
        NAPT_TABLE_FOREACH(napt_udp, napt_table_4udp)
        {
            if (port == napt_udp->new_port)
            {
                is_used = true;
                break;
            }
        }
#ifdef NAPT_TABLE_MUTEX_LOCK
        alg_napt_unlock(napt_table_lock_4udp);
#endif
    }

    return is_used;
}

/*****************************************************************************
 Prototype    : alg_napt_init
 Description  : Network Address Port Translation£¨napt£©initialize
 Input        : void
 Output       : None
 Return Value : int      0   success
                     other   fail
 ------------------------------------------------------------------------------
 
  History        :
  1.Date         : 2015/3/10
    Author       : Li Limin, lilm@winnermicro.com
    Modification : Created function

*****************************************************************************/
int alg_napt_init(void)
{
    int err = 0;
#ifdef NAPT_USE_HW_TIMER
    struct tls_timer_cfg timer_cfg;
#endif

    memset(&napt_table_4tcp, 0, sizeof(struct napt_table_head_4tu));
    memset(&napt_table_4udp, 0, sizeof(struct napt_table_head_4tu));
    memset(&napt_table_4ic, 0, sizeof(struct napt_table_head_4ic));

    napt_curr_port = NAPT_LOCAL_PORT_RANGE_START;
    napt_curr_id   = NAPT_ICMP_ID_RANGE_START;

#ifdef NAPT_TABLE_MUTEX_LOCK
    err = tls_os_sem_create(&napt_table_lock_4tcp, 1);
    if (TLS_OS_SUCCESS != err)
    {
        NAPT_PRINT("failed to init alg.\n");
        return err;
    }

    err = tls_os_sem_create(&napt_table_lock_4udp, 1);
    if (TLS_OS_SUCCESS != err)
    {
        NAPT_PRINT("failed to init alg.\n");
        return err;
    }

    err = tls_os_sem_create(&napt_table_lock_4ic, 1);
    if (TLS_OS_SUCCESS != err)
    {
        NAPT_PRINT("failed to init alg.\n");
    }
#endif

#ifdef NAPT_ALLOC_DEBUG
    napt4ic_cnt = 0;
    napt4tcp_cnt = 0;
    napt4udp_cnt = 0;
#endif

#ifdef NAPT_USE_HW_TIMER
    memset(&timer_cfg, 0, sizeof(timer_cfg));
    timer_cfg.unit = TLS_TIMER_UNIT_MS;
    timer_cfg.timeout = NAPT_TMR_INTERVAL;
    timer_cfg.is_repeat = TRUE;
    timer_cfg.callback = alg_napt_table_check;
    err = tls_timer_create(&timer_cfg);
    if (WM_TIMER_ID_INVALID != err)
    {
        tls_timer_start(err);
        err = 0;
    }
    else
    {
        NAPT_PRINT("failed to init alg timer.\n");
    }
#else
    sys_timeout(NAPT_TMR_INTERVAL, alg_napt_table_check, (void *)NAPT_TMR_INTERVAL);
#endif

    return err;
}

#endif

