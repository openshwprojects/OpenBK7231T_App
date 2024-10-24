#include "wm_config.h"

#include <string.h>
#include "wm_debug.h"
#include "wm_socket_fwup.h"
#include "list.h"
#include "wm_fwup.h"
#include "wm_sockets.h"
#include "wm_mem.h"

#define SOCKET_FWUP_PACK_SIZE 2048
static struct socket_fwup_pack current_pack;
static u16 current_offset;
static u32 session_id;

static void free_current_pack()
{
	session_id = 0;
	if(current_pack.data != NULL)
	{
		tls_mem_free(current_pack.data);
		memset(&current_pack, 0, sizeof(struct socket_fwup_pack));
	}
}

s8  socket_fwup_recv(u8 skt_num, struct pbuf *p, s8 err)
{
	u16 copylen;
	int offset = 0;
	int ret = 0;
	do
	{
		if(current_pack.operation < SOCKET_FWUP_START)
		{
			current_offset = 0;
			copylen = pbuf_copy_partial(p, &current_pack.operation, 1, offset);
			//TLS_DBGPRT_ERR("operation = %d, p->tot_len = %d\n", current_pack.operation, p->tot_len );
			offset += copylen;
			copylen = pbuf_copy_partial(p, &current_pack.datalen, 2, offset);
			//TLS_DBGPRT_ERR("datalen copylen = %d\n", copylen);
			current_pack.datalen = ntohs(current_pack.datalen);
			TLS_DBGPRT_ERR("datalen = %d\n", current_pack.datalen);
			offset += copylen;
			if(current_pack.operation > SOCKET_FWUP_DATA || current_pack.operation < SOCKET_FWUP_START)
			{
				TLS_DBGPRT_ERR("current_pack.operation = %d is invalid\n", current_pack.operation);
				goto err;//return ERR_ABRT;
			}
			if(current_pack.operation == SOCKET_FWUP_START)
			{
				session_id = tls_fwup_enter(TLS_FWUP_IMAGE_SRC_WEB);
				if(session_id == 0)
				{
					TLS_DBGPRT_ERR("session_id = %d is invalid\n", session_id);
					goto err;//return ERR_ABRT;
				}
			}

			if (current_pack.datalen == 0)
			{
				//tls_socket_close(skt_num);
				goto err;//return ERR_ABRT;
			}			
		}
		else if(current_pack.datalen > 0 && current_pack.datalen <= SOCKET_FWUP_PACK_SIZE)
		{
			copylen = pbuf_copy_partial(p, &current_pack.data[current_offset], current_pack.datalen-current_offset, offset);
			//TLS_DBGPRT_INFO("copylen=%d, current_pack.data = %02x%02x%02x%02x%02x%02x", copylen, current_pack.data[0], current_pack.data[1], current_pack.data[2], current_pack.data[3], current_pack.data[4], current_pack.data[5]);
			offset += copylen;
			current_offset += copylen;
			if(current_offset == current_pack.datalen)
			{
				ret = tls_fwup_request_sync(session_id, current_pack.data, current_pack.datalen);
				if(ret != TLS_FWUP_STATUS_OK)
				{
					TLS_DBGPRT_ERR("up data error.\n");
				    if (p)
				    {
				        pbuf_free(p);					
				    }
					return  ERR_VAL;
				}
				
				if(current_pack.operation == SOCKET_FWUP_END)
				{
#if TLS_CONFIG_SOCKET_RAW
					tls_socket_close(skt_num);
#endif
					tls_fwup_exit(session_id);
					free_current_pack();
					break;
				}
				current_pack.operation = 0;
 			}
		}
		else if(current_pack.datalen == 0)
		{
			if(current_pack.operation == SOCKET_FWUP_END)
			{
#if TLS_CONFIG_SOCKET_RAW
				tls_socket_close(skt_num);
#endif
				tls_fwup_exit(session_id);
				free_current_pack();
				break;
			}
			current_pack.operation = 0;
		}
	}while(offset < p->tot_len);
	if (p)
            pbuf_free(p);
	return ERR_OK;

err:
    if (p)
        pbuf_free(p);
    return ERR_ABRT;
}

void  socket_fwup_err(u8 skt_num, s8 err)
{
	TLS_DBGPRT_INFO("socket num=%d, err= %d\n", skt_num, err);
	tls_fwup_exit(session_id);
	free_current_pack();
}

s8 socket_fwup_poll(u8 skt_num)
{
	//printf("socketpoll skt_num : %d\n", skt_num);
	return ERR_OK;
}

s8 socket_fwup_accept(u8 skt_num, s8 err)
{
	TLS_DBGPRT_INFO("accept socket num=%d, err= %d\n", skt_num, err);
	if(ERR_OK == err)
	{
		//ret = tls_fwup_init();
		//if(ret == TLS_FWUP_STATUS_EMEM)
		//	return ERR_ABRT;
		if(session_id == 0 || tls_fwup_current_state(session_id) == TLS_FWUP_STATE_UNDEF)
		{
			memset(&current_pack, 0, sizeof(struct socket_fwup_pack));
			current_pack.data = tls_mem_alloc(SOCKET_FWUP_PACK_SIZE);
			if(NULL == current_pack.data)
			{
				printf("\nmalloc socket fwup pack fail\n");
				return ERR_ABRT;
			}
			memset(current_pack.data, 0, SOCKET_FWUP_PACK_SIZE);
			return ERR_OK;
		}
		else
		{
			TLS_DBGPRT_ERR("session_id=%d is not current session id\n", session_id);
			return ERR_ABRT;
		}
	}
	return err;
}

