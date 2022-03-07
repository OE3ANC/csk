/*
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <zephyr.h>
#include <sys/printk.h>

#include <tester_lisa.h>
#include <lisa_http.h>

#include <logging/log.h>
LOG_MODULE_REGISTER(tester_http, LOG_LEVEL_DBG);



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


#if defined(CONFIG_NET_SOCKETS_SOCKOPT_TLS)
#define HTTP_PORT "443"
#else
#define HTTP_PORT "80"
#endif

/* HTTP server to connect to */
#define HTTP_HOST "www.rt-thread.com"
// #define HTTP_HOST "192.168.0.100" 





#define HEADERS "Content-Type: application/octet-stream\r\n"

lisa_http_t *test_http = NULL;

void
get_http_data_cb(lisa_http_data_t *data)
{
	printf("-----get_auth_data_cb -----");
	if (data->buf) {
		printf("%s", (char *)data->buf);
		printf("\n");
	}
	if (data->user) {
		printf("%p", (char *)data->user);
	}
}

const char *headers[] = {
		"Content-Type: application/octet-stream\r\n",
		NULL
	};

void lisa_https_test(void)
{

    _wifi_init();
	k_sleep(K_TICKS(50000));


	#if 1

	lisa_http_request_t http_param;
	lisa_http_err_e http_ret = 0;

	lisa_http_create();

	http_param.url = "/service/echo";
	http_param.host = "www.rt-thread.com";
	http_param.headers = headers;
	http_param.body = "RT-Thread is an open source IoT";
	http_param.body_len = strlen(http_param.body)+1;
	http_param.method = LISA_HTTP_POST;
	http_param.timeout_ms = 3 * MSEC_PER_SEC;
	http_param.on_data = get_http_data_cb;
	http_param.on_event = NULL;
	http_param.user = NULL;
	test_http = lisa_http_set(&http_param);
	if (test_http == NULL) {
	printf("lisa_http_set failed\n");
	}

	http_ret = lisa_http_perform(test_http);
	if (http_ret) {
		printf("lisa_http_perform failed ret = %d\n", http_ret);
	}
	k_sleep(K_TICKS(50000));
	lisa_http_cancel(test_http);
	test_http = NULL;

	lisa_http_cleanup();
	#endif

}