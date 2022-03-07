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
#include <net/http_client.h>

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

#include "lisa_http.h"

// #include <logging/log.h>
// LOG_MODULE_REGISTER(lisa_http, LOG_LEVEL_DBG);

#define TAG "http"

#define WS_DEBUG 1
#if WS_DEBUG
#define WS_DBG(fmt, ...)                                                                           \
	printf("[%s]%s:%d:%s: " fmt "\n", TAG, __FILE__, __LINE__, __FUNCTION__, ##__VA_ARGS__);
#else
#define WS_DBG(fmt, ...)
#endif

#if defined(CONFIG_NET_SOCKETS_SOCKOPT_TLS)
#define HTTP_PORT "443"
#else
#define HTTP_PORT "80"
#endif

#define RESP_BUF_SIZE 1024

typedef struct {
	bool enable_status;
	lisa_semaphore_t *sem;
	lisa_thread_t *thread;
	uint8_t *read_buffer;
	lisa_http_t *lisa_http;
} hal_http_mng_t;

hal_http_mng_t *g_http_mng = NULL;

static int setup_socket(sa_family_t family, int *sock)
{
	const char *family_str = family == AF_INET ? "IPv4" : "IPv6";
	int ret = 0;

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
	}

	return ret;
}

static int connect_socket(sa_family_t family, int *sock, struct sockaddr *addr, socklen_t addr_len)
{
	int ret;
	// net_sin(addr)->sin_port = htons(port);
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

static void response_cb(struct http_response *rsp, enum http_final_call final_data, void *user_data)
{
	lisa_http_t *ins = NULL;
	lisa_http_data_t http_data;
	lisa_http_event_t http_event;

	// if (final_data == HTTP_DATA_MORE) {
	// 	printf("Partial data received (%zd bytes)\n", rsp->data_len);
	// } else if (final_data == HTTP_DATA_FINAL) {
	// 	printf("All the data received (%zd bytes)\n", rsp->data_len);
	// }
	
	printf("Response status %s\n", rsp->http_status);
	printf("Response http_status_code  %d\n", rsp->http_status_code);

	if (g_http_mng->lisa_http) {
		ins = g_http_mng->lisa_http;

		if (200 != rsp->http_status_code) {
			http_event.error = LISA_HTTP_ERROR_NETWORK;
			if (ins->inter_on_event) {
				ins->inter_on_event(&http_event);
			}
			return;
		}
		printf("read_buffer %s\n",g_http_mng->read_buffer);
		printf("\n\n");
		http_data.buf = g_http_mng->read_buffer;
		http_data.len = rsp->data_len;
		http_data.user = ins->user;
		if (ins->inter_on_data){
			ins->inter_on_data(&http_data);
		}
		else
		{
			printf("inter_on_data null\n");
		}
	}
}

int lisa_http_create(void)
{
	if (g_http_mng) {
		printf("error: lisa_http is exist\n");
		return LISA_FAIL;
	}
	g_http_mng = (hal_http_mng_t *)malloc(sizeof(hal_http_mng_t));
	if (g_http_mng == NULL) {
		printf("no memory for g_http_mng.\n");
		return LISA_HTTP_COMMON_ERR;
	}
	memset(g_http_mng, 0, sizeof(hal_http_mng_t));
	g_http_mng->enable_status = true;

	g_http_mng->read_buffer = (unsigned char *)malloc(RESP_BUF_SIZE);
	if (g_http_mng->read_buffer == NULL) {
		printf("no memory for receive buffer.\n");
		goto out;
	}
	return LISA_OK;

out:
	free(g_http_mng);
	return LISA_FAIL;
}

lisa_http_t *lisa_http_set(lisa_http_request_t *req)
{
	// int ret = 0;
	int sock4 = -1;
	lisa_http_t *lisa_http_temp = NULL;

	if (!g_http_mng) {
		WS_DBG("error: lisa_http is not create\n");
		return NULL;
	}

	lisa_http_temp = (lisa_http_t *)malloc(sizeof(lisa_http_t));
	if (lisa_http_temp == NULL) {
		WS_DBG("no memory for lisa_http_t.\n");
		return NULL;
	}
	memset(lisa_http_temp, 0, sizeof(struct http_request));

	strcpy(lisa_http_temp->url, req->url);
	strcpy(lisa_http_temp->host, req->host);

	static struct addrinfo hints;
	struct addrinfo *res;
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	int st;

	st = getaddrinfo(lisa_http_temp->host, HTTP_PORT, &hints, &res);
	printf("getaddrinfo status: %d\n", st);

	int port = strtol(HTTP_PORT, NULL, 10);
	if (port < 1 || port > 65535) {
		printf("Err: DNS_EAI_NONAME \n");
		goto __err_exit;
	}

	if (IS_ENABLED(CONFIG_NET_IPV4)) {
		(void)connect_socket(AF_INET, &sock4, res->ai_addr, res->ai_addrlen);
	}

	if (sock4 < 0) {
		printf("Cannot create HTTP connection.");
		goto __err_exit;
	}
	if (req->body_len > 0) {
		memcpy(lisa_http_temp->body, req->body, req->body_len); 
	}

    if (req->headers)  
    {
        lisa_http_temp->headers = req->headers;
    }

	lisa_http_temp->sock_id = sock4;
	lisa_http_temp->body_len = strlen(req->body);
	lisa_http_temp->method = req->method;
	lisa_http_temp->timeout_ms = req->timeout_ms;
	lisa_http_temp->status_code = 0;
	lisa_http_temp->user = req->user;
	lisa_http_temp->inter_on_data = req->on_data;
	lisa_http_temp->inter_on_event = req->on_event;

	g_http_mng->lisa_http = lisa_http_temp;

	return lisa_http_temp;

__err_exit:
	free(lisa_http_temp);
	return NULL;
}


lisa_http_err_e lisa_http_perform(lisa_http_t *ins)
{
	lisa_http_err_e http_ret = 0;

	if (!ins) {
		WS_DBG("webclient error: ins is null\n");
		http_ret = LISA_HTTP_ERROR_PARAM;
		return http_ret;
	}
	if (ins->sock_id >= 0 && IS_ENABLED(CONFIG_NET_IPV4)) {
		struct http_request req;

		// const char *headers[] = { "Content-Type: application/octet-stream\r\n", NULL };
		memset(&req, 0, sizeof(req));

		if (ins->method == LISA_HTTP_GET) {
			req.method  = HTTP_GET;
		} else if (ins->method == LISA_HTTP_POST) {
			req.method  = HTTP_POST;
		} else {
			req.method  = 0;
			WS_DBG("lisa_http_temp->http_request.method null\n");
		}

		req.url = ins->url;
		req.host = ins->host;
		req.protocol = "HTTP/1.1";
		req.payload = ins->body;
		req.payload_len = ins->body_len;
		req.header_fields = ins->headers;
		req.response = response_cb;
		req.recv_buf = g_http_mng->read_buffer;
		req.recv_buf_len = RESP_BUF_SIZE;

        if(http_client_req(ins->sock_id, &req, ins->timeout_ms, NULL)<0)
        {
            WS_DBG("webclient error: http_client_req\n");
            http_ret = LISA_HTTP_ERROR_PARAM;
        }		
	}

	return http_ret;
}

lisa_http_err_e lisa_http_write(lisa_http_t *ins, const void *buffer, size_t length)
{
	// todo
	return LISA_OK;
}

lisa_http_err_e lisa_http_cancel(lisa_http_t *ins)
{
	lisa_http_err_e http_ret = 0;
	if (!ins) {
		WS_DBG("webclient error: ins is null\n");
		http_ret = LISA_HTTP_ERROR_PARAM;
		return http_ret;
	}
	if (ins->sock_id > 0) {
		close(ins->sock_id);
		ins->sock_id = -1;
		g_http_mng->lisa_http = NULL;
	}

	free(ins);
	return http_ret;
}

lisa_http_err_e lisa_http_cleanup(void)
{
	WS_DBG("lisa_http_cleanup----\n");
	lisa_http_err_e http_ret = 0;
	if (!g_http_mng) {
		WS_DBG("error: g_http_mng is not created\n");
		return LISA_HTTP_COMMON_ERR;
	}
	if (g_http_mng->lisa_http) {
		WS_DBG("info: g_http_mng->lisa_http is running,cancel it\n");
		http_ret = lisa_http_cancel(g_http_mng->lisa_http);
		if (http_ret) {
			WS_DBG("error: lisa_http_cancel error\n");
			return LISA_HTTP_COMMON_ERR;
		}
	}

	free(g_http_mng->read_buffer);
	free(g_http_mng);
	return http_ret;
};
