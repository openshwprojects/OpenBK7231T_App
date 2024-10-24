/**
 * @file    wm_socket2.1.3.h
 *
 * @brief   socket2.1.3 apis
 *
 * @author  winnermicro
 *
 * @copyright (c) 2014 Winner Microelectronics Co., Ltd.
 */
#ifndef WM_SOCKET2_1_3_H
#define WM_SOCKET2_1_3_H

#include "wm_type_def.h"
#include "wm_netif.h"
#include "lwip/err.h"
#include "lwip/pbuf.h"


//socket state defination
#define NETCONN_STATE_NONE         0
#define NETCONN_STATE_WAITING      1
#define NETCONN_STATE_CONNECTED    2
#define NETCONN_STATE_CLOSED       3

//socket event defination
#define NET_EVENT_TCP_JOINED            0
#define NET_EVENT_TCP_DISCONNECT        1
#define NET_EVENT_TCP_CONNECTED         2
#define NET_EVENT_TCP_CONNECT_FAILED    3
#define NET_EVENT_UDP_START             4
#define NET_EVENT_UDP_START_FAILED      5

#define TLS_MAX_SOCKET_NUM       4
#define TLS_MAX_NETCONN_NUM      20

/**
* @brief This Function prototype for tcp error callback functions. Called when 
*                    receives a RST or is unexpectedly closed for any other reason.
*                   The corresponding socket is already freed when this callback is called!
*
* @param[in] skt_num    Is the socket number that returned by tls_socket_create function.
*
* @param[in]  err           Error code to indicate why the socket has been closed
*                        ERR_ABRT: aborted through returning ERR_ABRT from within others
*                                                      callback functions
*                        ERR_RST: the connection was reset by the remote host
*/
typedef void  (*socket_err_fn)(u8 skt_num, err_t err);

/**
* @brief This Function prototype for socket receive callback functions. Called when data has
*                    been received.
*
* @param[in] skt_num   Is the socket number that returned by tls_socket_create function.
*
* @param[in] p         The received data (or NULL when the connection has been closed!)
*
* @param[in] err       An error code if there has been an error receiving, always be ERR_OK 
*                                    when cs mode is udp.
*
* @retval 			The return value is only valid for tcp receive, for upd it means nothing.
*                   ERR_OK: Return this value after handling the received data.
*                   ERR_ABRT:  Only return ERR_ABRT if you want to abort the socket from within the
*                                      callback function!
*/
typedef err_t  (*socket_recv_fn)(u8 skt_num, struct pbuf *p, err_t err);

/** 
* @brief This Function prototype for socket srce ip callback functions. Called when data has
*                    been received.
*
* @param[in] skt_num   Is the socket number that returned by tls_socket_create function.
*
* @param[in] datalen           The received data length
*
* @param[in] ipsrc         source ip addr
*
* @param[in] port          source port
*
* @param[in] err           An error code if there has been an error receiving, always be ERR_OK 
*                                    when cs mode is udp.
*
* @retval			The return value is only valid for UDP receive, for udp it means nothing.
*                   ERR_OK: Return this value after handling the received data.
*                   ERR_ABRT:  Only return ERR_ABRT if you want to abort the socket from within the
*                                      callback function!
*/
typedef err_t  (*socket_recv_ip_rpt_fn)(u8 skt_num, u16 datalen, u8 *ipsrc, u16 port, err_t err);

/**
* @brief This Function prototype for tcp connected callback functions. Called when 
*                   connected to the remote side.
*
* @param[in] skt_num   Is the socket number that returned by tls_socket_create function.
*
* @param[in] err       An unused error code, always ERR_OK currently.
*
* @retval  ERR_OK: Return this value after handling your logic.
* @retval  ERR_ABRT:  Only return ERR_ABRT if you want to abort the socket from within the
*                                      callback function!
*/
typedef err_t (*socket_connected_fn)(u8 skt_num,  err_t err);

/**
* @brief This Function prototype for tcp poll callback functions. Called periodically.
*
* @param[in] skt_num   Is the socket number that returned by tls_socket_create function.
*
* @retval  ERR_OK:     Try to do something periodically.
* @retval  ERR_ABRT:  Only return ERR_ABRT if you want to abort the socket from within the
*                                      callback function!
*/
typedef err_t (*socket_poll_fn)(u8 skt_num);

/**
* @brief This Function prototype for tcp accept callback functions. Called when a new
*                   connection can be accepted on a listening tcp.
*
* @param[in] skt_num   Is the socket number that returned by tls_socket_create function.
*
* @param[in] err       An error code if there has been an error accepting.
*
* @retval  ERR_OK:      Return this value after handling your logic.
* @retval  ERR_ABRT:  Only return ERR_ABRT if you want to abort the socket from within the
*                                      callback function!
*/
typedef err_t (*socket_accept_fn)(u8 skt_num, err_t err);

/**
* @brief This Function prototype for socket state changed callback functions. Called when socket
*                   the sockte's state changed.
*
* @param[in] skt_num   Is the socket number that returned by tls_socket_create function.
*
* @param[in] event     Is the event number, see socket event defination.
*
* @param[in] state     Is the socket state, see socket state defination.
*/
typedef void(*socket_state_changed_fn)(u8 skt_num, u8 event, u8 state);

enum tls_socket_protocol{
    SOCKET_PROTO_TCP,      /* TCP Protocol    */
    SOCKET_PROTO_UDP,     /* UDP Protocol   */
};

enum tls_socket_cs_mode{
    SOCKET_CS_MODE_CLIENT,    /* Client mode    */
    SOCKET_CS_MODE_SERVER,    /* Server mode   */
};

struct tls_socket_desc {
    enum tls_socket_cs_mode cs_mode;              /* Server mode  or Client mode, Only for tcp protocol is valid */
    enum tls_socket_protocol protocol;                /* TCP Protocol or UDP Protocol  */
    ip_addr_t  ip_addr;                                              /* Remote ip address, for tcp client mode is remote server's ip address; for tcp server mode can be any address. */
	                                                                    /*          for udp is remote server's ip address */
    u16 port;                                                       /* port, for tcp client mode is remote server's port; for tcp server mode is local listen port . 
	                                                                              for udp is remote server's port */
    u16 localport;                                               /* local port, for udp and tcp client is local listen port, for tcp server means nothing, tcp server always listen at port */
    char host_name[32];                                     /* remote host name, not support for now  */
    u8  host_len;                                                 /* the length of host name   */
    u32 timeout;                                                  /* poll timeout, not implemented for now   */
    socket_err_fn errf;                                          /* a pointer to socket_err_fn   */
    socket_recv_fn recvf;                                      /* a pointer to socket_recv_fn  */
    socket_connected_fn connf;                             /* a pointer to socket_connected_fn  */
    socket_poll_fn pollf;                                        /* a pointer to socket_poll_fn  */
    socket_accept_fn acceptf;                               /* a pointer to socket_accept_fn   */
    socket_state_changed_fn state_changed;       /* a pointer to socket_state_changed_fn   */
    socket_recv_ip_rpt_fn recvwithipf;           /*recv skt info report*/
};

/**
* @brief This function is called by your application code to create a socket.
*
* @param[in] skd      Is a pointer to an tls_socket_desc.
*
* @retval	 ERR_OK    If create socket successfully.
*              negative number   If an error was detected.
*/
int tls_socket_create(struct tls_socket_desc * skd);

/**
* @brief This function is called by your application code to send data by the socket.
*
* @param[in] skt_num      Is the socket number that returned by tls_socket_create function.
*
* @param[in] pdata          Is a pointer to the data which need to be send by the socket.
*
* @param[in] len              The data's length.
*
* @retval	 ERR_OK    If send data successfully.
*              negative number   If an error was detected.
*/
int tls_socket_send(u8 skt_num, void *pdata, u16 len);

/**
* @brief This function is called by your application code to close the socket, and the related resources would be released.
*
* @param[in] skt_num      Is the socket number that returned by tls_socket_create function.
*
* @retval	 ERR_OK    If close socket successfully.
*              negative number   If an error was detected.
*/
int tls_socket_close(u8 skt_num);

struct tls_skt_status_ext_t {
    u8 socket;
    u8 status;
    enum tls_socket_protocol protocol;
    u8 host_ipaddr[4];
    u16 remote_port;
    u16 local_port;
};

struct tls_skt_status_t {
    u32 socket_cnt;
    struct tls_skt_status_ext_t skts_ext[1];
};
/**
* @brief This function is called by your application code to get the socket status of specified socket num.
*
* @param[in] skt_num      Is the socket number that returned by tls_socket_create function.
*
* @param[in] buf          Is a pointer to the data contains the socket status, if the socket is server, also contains it's client's status.
*
* @param[in] len          The buf's length. At least, the len should be bigger than sizeof(struct tls_skt_status_t).
*
* @retval	 	ERR_OK    If send data successfully.
*              negative number   If an error was detected.
*/
int tls_socket_get_status(u8 skt_num, u8 *buf, u32 bufsize);

/**
* @brief This function is called by your application code to send data by udp socket.
*
* @param[in] localport         This function will search all created sockets, if there is a socket whose localport equals this value and it's protocol is udp,
*                                          then send the data by this socket, otherwise, nothing to send.
*
* @param[in] ip_addr          Is the remote ip address.
*
* @param[in] port               Is the remote port which upd send to.
*
* @param[in] pdata             Is a pointer to the data which need to be send by the socket.
*
* @param[in] len              The data's length.
* @retval	 	ERR_OK    If send data successfully.
*              negative number   If an error was detected.
*/
int tls_socket_udp_sendto(u16 localport, u8  *ip_addr, u16 port, void *pdata, u16 len);

#endif

