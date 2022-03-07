/*
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <zephyr.h>
#include <sys/printk.h>

#include <tester_lisa.h>
#include <lisa_websocket.h>

#include <logging/log.h>
LOG_MODULE_REGISTER(tester_websocket, LOG_LEVEL_DBG);

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


// ws://82.157.123.54:9010/ajaxchattest

#define WS_TEST_HOST_1 "192.168.0.100"
#define WS_TEST_HOST_2 "ivs.iflyos.cn"
#define WS_TEST_PORT_1 "9010"
#define WS_TEST_PORT_2 "80"
#define WS_TEST_PATH_1 "/ajaxchattest"
#define WS_TEST_PATH_2 "/embedded/v1?device_id=12345&token=0000000000000000"



void test_ws_data_cb(lisa_ws_data_t *data)
{
	printf("data_cb.  ");
	if (data->type == LISA_WS_TEXT) {
		printf("LISA_WS_TEXT:");
		if (data->buf) {
			printf("%s", (char *)data->buf);
			printf("\n");
		}
	} else if (data->type == LISA_WS_BIN) {
		printf("LISA_WS_BIN");
        printf("\n");
	}
}

void test_ws_event_cb(lisa_ws_event_t *data)
{
	printf("event_cb. %d \n", data->what);
}

uint8_t bin_data[] = { 1, 2, 3, 4, 5, 6, 7, 8, 9 };
char text[] = "hello tls!";



lisa_ws_t *lisa_ws = NULL;

void lisa_websocket_test(void)
{
	int ret = LISA_FAIL;
	_wifi_init();
	k_sleep(K_TICKS(50000));
	k_sleep(K_TICKS(50000));

	lisa_ws_request_t lisa_ws_req = { 0 };

	lisa_ws_req.host = WS_TEST_HOST_1;
	lisa_ws_req.port = WS_TEST_PORT_1;
	lisa_ws_req.path = WS_TEST_PATH_1;
	lisa_ws_req.timeout_ms = 10 * MSEC_PER_SEC;
	lisa_ws_req.on_data = test_ws_data_cb;
	lisa_ws_req.on_event = test_ws_event_cb;
	lisa_ws_req.user = NULL;
	lisa_ws = lisa_ws_init(&lisa_ws_req);
    if(lisa_ws)
    {
        ret = lisa_ws_connect(lisa_ws);
    }
	if (ret == LISA_OK) {       

		k_sleep(K_TICKS(10000));
		lisa_ws_send_text(lisa_ws, text);

        k_sleep(K_TICKS(10000));
		lisa_ws_send_text(lisa_ws, text);
		k_sleep(K_TICKS(50000));
		lisa_ws_send_binary(lisa_ws, bin_data, sizeof(bin_data));

		k_sleep(K_TICKS(50000));
		lisa_ws_disconnect(lisa_ws);
		lisa_ws_cleanup(lisa_ws);
	}
}


