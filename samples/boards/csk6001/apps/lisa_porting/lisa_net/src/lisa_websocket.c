/*
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <zephyr.h>
#include <sys/printk.h>

#include <net/net_ip.h>
#include <net/socket.h>
#include <net/websocket.h>
#include <net/http_client.h>

#include "lisa_websocket.h"

#if !defined(__ZEPHYR__) || defined(CONFIG_POSIX_API)

#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netdb.h>

#else

#include <net/socket.h>
#include <kernel.h>

#if defined(CONFIG_NET_SOCKETS_SOCKOPT_TLS)
#include <net/tls_credentials.h>
#include "ca_certificate.h"
#endif

#endif

#define TAG "lisa_websocket"

#define WS_DEBUG 1
#if WS_DEBUG
#define WS_DBG(fmt, ...)                                                                           \
	printf("[%s]%s,line:%d: " fmt "\n", TAG, __FUNCTION__, __LINE__, ##__VA_ARGS__);
#else
#define WS_DBG(fmt, ...)
#endif

#define WS_THREAD_STACK_SIZE (1 * 1024)
K_THREAD_STACK_DEFINE(ws_recv_stack, WS_THREAD_STACK_SIZE);
#define WS_THREAD_PRIORITY  2

#define WS_RECV_BUF_SIZE 1024
static uint8_t ws_recv_buf[WS_RECV_BUF_SIZE];

#define RESP_BUF_SIZE 1024
static uint8_t recv_buf_ipv4[RESP_BUF_SIZE];

typedef struct {
	int sock_id;
	int ws_sock;
	k_tid_t recv_t_id;
} lisa_ws_mng_t;

static int setup_socket(sa_family_t family, int *sock)
{
	const char *family_str = family == AF_INET ? "IPv4" : "IPv6";
	int ret = 0;

	struct timeval optval = {
		.tv_sec = 10,
		.tv_usec = 0,
	};

#if defined(CONFIG_NET_SOCKETS_SOCKOPT_TLS)
	sec_tag_t sec_tag_list[] = {
		CA_CERTIFICATE_TAG,
	};

	*sock = socket(family, SOCK_STREAM, IPPROTO_TLS_1_2);
	if (*sock >= 0) {
		ret = setsockopt(*sock, SOL_TLS, TLS_SEC_TAG_LIST, sec_tag_list,
				 sizeof(sec_tag_list));
		if (ret < 0) {
			printf("Failed to set %s secure option (%d)", family_str, -errno);
			ret = -errno;
		}

		ret = setsockopt(*sock, SOL_TLS, TLS_HOSTNAME, TLS_PEER_HOSTNAME,
				 sizeof(TLS_PEER_HOSTNAME));
		if (ret < 0) {
			printf("Failed to set %s TLS_HOSTNAME "
			       "option (%d)",
			       family_str, -errno);
			ret = -errno;
		}
	}

#else
	*sock = socket(family, SOCK_STREAM, IPPROTO_TCP);
#endif
	if (*sock < 0) {
		printf("Failed to create %s HTTP socket (%d)", family_str, -errno);
		return ret;
	}
	printf("socket finish\n");
	setsockopt(*sock, SOL_SOCKET, SO_SNDTIMEO, &optval,
			sizeof(optval));
	return ret;
}



static int connect_socket(sa_family_t family, int *sock, struct sockaddr *addr, socklen_t addr_len)
{
	int ret;

	ret = setup_socket(family, sock);
	if (ret < 0 || *sock < 0) {
		return -1;
	}

	ret = connect(*sock, addr, addr_len);
	if (ret < 0) {
		printf("Cannot connect to %s remote (%d)", family == AF_INET ? "IPv4" : "IPv6",
		       -errno);
		ret = -errno;
	}

	return ret;
}


void lisa_ws_thread(void *arg)
{
	uint64_t remaining = 0;
	uint32_t message_type;
	int ret = 0;
	lisa_ws_data_t ws_data;
	lisa_ws_event_t ws_event;
	lisa_ws_t *ins = (lisa_ws_t *)arg;

	if (ins->inter_on_event) {
		ws_event.what = LISA_WS_ON_CONNECTED;
		ws_event.user = ins->user;
		ins->inter_on_event(&ws_event);
	}
	lisa_ws_mng_t *ws_mng = (lisa_ws_mng_t*)ins->ws_mngt;
	while (1) {
		remaining = 0;
		memset(ws_recv_buf, 0, sizeof(ws_recv_buf));
		ret = websocket_recv_msg(ws_mng->ws_sock, ws_recv_buf, 1, &message_type, &remaining,
					0);
					//  SYS_FOREVER_MS);

		if (ret < 0) {
			// WS_DBG("websocket_recv_msg error (%d)", ret);
			k_sleep(K_MSEC(1000));
			continue;
		}
		if (remaining > 0) {
			printf("remaining: %lld \n", remaining);
			ret = websocket_recv_msg(ws_mng->ws_sock, &ws_recv_buf[1], remaining,
						 &message_type, NULL, 0);
		}
		if (ret < 0) {
			WS_DBG("websocket_recv_msg2 error (%d)", ret);
			continue;
		} 

		WS_DBG("websocket_recv_msg ---%d \n", message_type);
		#if 1
		if (message_type & WEBSOCKET_FLAG_CLOSE) {
			lisa_ws_disconnect(ins);

			if (ins->inter_on_event) {
				ws_event.what = LISA_WS_ON_DISCONNECTED;
				ws_event.user = ins->user;
				ins->inter_on_event(&ws_event);
			}
		} else if (message_type & WEBSOCKET_FLAG_TEXT) {
			// if (ins->inter_on_event) {
			// 	ws_event.what = LISA_WS_ON_DATA;
			// 	ws_event.user = ins->user;
			// 	ins->inter_on_event(&ws_event);
			// }
			if (ins->inter_on_data) {
				ws_data.type = LISA_WS_TEXT;
				ws_data.buf = ws_recv_buf;
				ws_data.len = 1 + remaining;
				ins->inter_on_data(&ws_data);
			}
		} else if (message_type & WEBSOCKET_FLAG_BINARY) {
			// if (ins->inter_on_event) {
			// 	ws_event.what = LISA_WS_ON_DATA;
			// 	ws_event.user = ins->user;
			// 	ins->inter_on_event(&ws_event);
			// }
			if (ins->inter_on_data) {
				ws_data.type = LISA_WS_BIN;
				ws_data.buf = ws_recv_buf;
				ws_data.len = 1 + remaining;
				ins->inter_on_data(&ws_data);
			}
		} else {
			if (ins->inter_on_event) {
				ws_event.what = LISA_WS_ON_ERROR;
				ws_event.user = ins->user;
				ins->inter_on_event(&ws_event);
			}
		}
		#endif
	}
}

static int connect_cb(int sock, struct http_request *req, void *user_data)
{
	// this callback  not means connecting is finished
	printf(" -----------------connect_cb finish %d\n", sock);

	return 0;
}

lisa_ws_t *lisa_ws_init(lisa_ws_request_t *req)
{
	WS_DBG("lisa_ws_init");

	lisa_ws_t *ins = malloc(sizeof(lisa_ws_t));
	if (ins == NULL) {
		WS_DBG("ins create failed. ");
		return NULL;
	}

	lisa_ws_mng_t *ws_mng = malloc(sizeof(lisa_ws_mng_t));
	if (ws_mng == NULL) {
		WS_DBG("ws_mng create failed. ");
		return NULL;
	}

	strcpy((char *)ins->u_info.scheme, req->scheme);
	strcpy((char *)ins->u_info.host, req->host);
	strcpy((char *)ins->u_info.path, req->path);
	strcpy((char *)ins->u_info.port, req->port);

	ins->user = req->user;
	ins->inter_on_event = req->on_event;
	ins->inter_on_data = req->on_data;
	ins->timeout_ms = req->timeout_ms;
	ins->ws_status = LISA_WS_STATUS_NULL;

	static struct addrinfo hints;
	struct addrinfo *res;
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	int st;
	int sock4 = -1;


	st = getaddrinfo(ins->u_info.host, ins->u_info.port, &hints, &res);
	printf("getaddrinfo status: %d\n", st);

	if (IS_ENABLED(CONFIG_NET_IPV4)) {
		(void)connect_socket(AF_INET, &sock4, res->ai_addr, res->ai_addrlen);
	}

	if (sock4 < 0) {
		printf("Cannot create HTTP connection.");
		goto __err_exit;
	}
	ws_mng->sock_id = sock4;
	
	ins->ws_mngt = (void *)ws_mng;
	return ins;

__err_exit:
	free(ins);
	return NULL;
}

lisa_ws_err_e lisa_ws_connect(lisa_ws_t *ins)
{
	lisa_err_t ret = LISA_FAIL;
	WS_DBG("lisa_ws_connect. ");
	struct websocket_request req;
	memset(&req, 0, sizeof(req));
	req.host = ins->u_info.host;
	req.url = ins->u_info.path;
	req.optional_headers = NULL;
	req.cb = connect_cb;
	req.tmp_buf = recv_buf_ipv4;
	req.tmp_buf_len = RESP_BUF_SIZE;	

	WS_DBG("req.host %s", req.host);
	WS_DBG("req.path %s", req.url);

	lisa_ws_mng_t *ws_mng = (lisa_ws_mng_t*)ins->ws_mngt;
	if (ws_mng->sock_id < 0) {
		printf("Csock_id invalid\n");
		return ret;
	}
	ws_mng->ws_sock = websocket_connect(ws_mng->sock_id, &req, ins->timeout_ms, (void *)ins);
	if (ws_mng->ws_sock < 0) {
		printf("Cannot connect to server\n");
		return ret;
	}
	printf("ins->ws_sock  =  %d\n", ws_mng->ws_sock);

	struct timeval optval = {
		.tv_sec = 10,
		.tv_usec = 0,
	};
	setsockopt(ws_mng->ws_sock, SOL_SOCKET, SO_RCVTIMEO, &optval,
			sizeof(optval));	

	static struct k_thread thread_handle;
	ws_mng->recv_t_id = k_thread_create(&thread_handle, ws_recv_stack, K_THREAD_STACK_SIZEOF(ws_recv_stack),
			(k_thread_entry_t)lisa_ws_thread, (void *)ins, NULL, NULL,
			WS_THREAD_PRIORITY, 0, K_NO_WAIT);

	return LISA_OK;
}

lisa_ws_err_e lisa_ws_send_text(lisa_ws_t *ins, const uint8_t *text)
{
	printf("text \r\n ");
	if (ins == NULL) {
		WS_DBG("ins is null ");
		return LISA_WS_COMMON_ERR;
	}
	// if (ins->ws_status != LISA_WS_STATUS_OK) {
	// 	WS_DBG("ws_status not connected");
	// 	return LISA_WS_COMMON_ERR;
	// }
	lisa_ws_mng_t *ws_mng = (lisa_ws_mng_t*)ins->ws_mngt;
	int send_len = websocket_send_msg(ws_mng->ws_sock, text, strlen(text),
					  WEBSOCKET_OPCODE_DATA_TEXT, true, true, ins->timeout_ms);
	if (send_len < 0) {
		WS_DBG("ws send text failed  ret = %d ", send_len);
		return LISA_WS_COMMON_ERR;
	}
	return LISA_WS_OK;
}

lisa_ws_err_e lisa_ws_send_binary(lisa_ws_t *ins, const void *buf, uint32_t len)
{
	printf("bin \r\n ");
	if (ins == NULL) {
		WS_DBG("ins is null ");
		return LISA_WS_COMMON_ERR;
	}

	// if (ins->ws_status != LISA_WS_STATUS_OK) {
	// 	WS_DBG("ws_status not connected");
	// 	return LISA_WS_COMMON_ERR;
	// }
	lisa_ws_mng_t *ws_mng = (lisa_ws_mng_t*)ins->ws_mngt;
	int send_len = websocket_send_msg(ws_mng->ws_sock, buf, len, WEBSOCKET_OPCODE_DATA_BINARY,
					  true, true, ins->timeout_ms);
	if (send_len < 0) {
		WS_DBG("ws send bin failed  ret = %d ", send_len);
		return LISA_WS_COMMON_ERR;
	}
	return LISA_WS_OK;
}

lisa_ws_err_e lisa_ws_disconnect(lisa_ws_t *ins)
{
	// lisa_err_t error = LISA_FAIL;
	WS_DBG("lisa_ws_disconnect. ");
	if (ins == NULL) {
		WS_DBG("ins is null ");
		return LISA_WS_COMMON_ERR;
	}
	lisa_ws_mng_t *ws_mng = (lisa_ws_mng_t*)ins->ws_mngt;
	k_thread_abort(ws_mng->recv_t_id);

	ins->ws_status = LISA_WS_STATUS_NOT_OK; // 记录状态
	if (ws_mng->ws_sock >= 0) {
		close(ws_mng->ws_sock);
		ws_mng->ws_sock = -1;
	}
	return LISA_OK;
}

lisa_ws_err_e lisa_ws_cleanup(lisa_ws_t *ins)
{
	// lisa_err_t error = LISA_FAIL;
	WS_DBG("lisa_ws_cleanup. ");
	if (ins == NULL) {
		WS_DBG("ins is null ");
		return LISA_WS_COMMON_ERR;
	}
	lisa_ws_mng_t *ws_mng = (lisa_ws_mng_t*)ins->ws_mngt;

	lisa_ws_disconnect(ins);

	if (ws_mng->sock_id >= 0) {
		close(ws_mng->sock_id);
		ws_mng->sock_id = -1;
	}

	free(ws_mng);
	free(ins);

	return LISA_OK;
}