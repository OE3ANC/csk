/*
 * Copyright (c) 2023, Emna Rekik
 * Copyright (c) 2024, Nordic Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>

#include <zephyr/kernel.h>
#include <zephyr/net/tls_credentials.h>
#include <zephyr/net/http/server.h>
#include <zephyr/net/http/service.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/net_ip.h>
#include <zephyr/net/socket.h>
#include <zephyr/net/dhcpv4_server.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(net_http_server_sample, LOG_LEVEL_DBG);

struct app_msg {
	uint32_t id;
	uint32_t len;
	void *data;
};

K_MSGQ_DEFINE(app_msgq, sizeof(struct app_msg), 4, 4);

static uint8_t index_html_gz[] = {
#include "index.html.gz.inc"
};

static uint16_t test_http_service_port = CONFIG_NET_SAMPLE_HTTP_SERVER_SERVICE_PORT;
HTTP_SERVICE_DEFINE(test_http_service, CONFIG_NET_CONFIG_MY_IPV4_ADDR, &test_http_service_port, 1,
		    10, NULL);

struct http_resource_detail_static index_html_gz_resource_detail = {
	.common = {
			.type = HTTP_RESOURCE_TYPE_STATIC,
			.bitmask_of_supported_http_methods = BIT(HTTP_GET),
			.content_encoding = "gzip",
			.content_type = "text/html",
		},
	.static_data = index_html_gz,
	.static_data_len = sizeof(index_html_gz),
};

HTTP_RESOURCE_DEFINE(index_html_gz_resource, test_http_service, "/",
		     &index_html_gz_resource_detail);

static uint8_t recv_buffer[1024];

static int dyn_handler(struct http_client_ctx *client, enum http_data_status status,
		       uint8_t *buffer, size_t len, void *user_data)
{
#define MAX_TEMP_PRINT_LEN 32
	static char print_str[MAX_TEMP_PRINT_LEN];
	enum http_method method = client->method;
	static size_t processed;

	__ASSERT_NO_MSG(buffer != NULL);

	if (status == HTTP_SERVER_DATA_ABORTED) {
		LOG_DBG("Transaction aborted after %zd bytes.", processed);
		processed = 0;
		return 0;
	}

	processed += len;

	snprintf(print_str, sizeof(print_str), "%s received (%zd bytes)",
		 http_method_str(method), len);
	LOG_HEXDUMP_DBG(buffer, len, print_str);

	if (status == HTTP_SERVER_DATA_FINAL) {
		LOG_DBG("All data received (%zd bytes).", processed);
		processed = 0;
		struct app_msg msg;
		k_msgq_put(&app_msgq, &msg, K_FOREVER);
	}

	/* This will echo data back to client as the buffer and recv_buffer
	 * point to same area.
	 */
	return len;
}

struct http_resource_detail_dynamic dyn_resource_detail = {
	.common = {
		.type = HTTP_RESOURCE_TYPE_DYNAMIC,
		.bitmask_of_supported_http_methods =
			BIT(HTTP_GET) | BIT(HTTP_POST),
	},
	.cb = dyn_handler,
	.data_buffer = recv_buffer,
	.data_buffer_len = sizeof(recv_buffer),
	.user_data = NULL,
};

HTTP_RESOURCE_DEFINE(dyn_resource, test_http_service, "/submit", &dyn_resource_detail);

extern int wifi_init(void);
extern int wifi_softAP_start(void);
extern int wifi_sta_connect(void);
extern int wifi_softAP_stop(void);
int main(void)
{
	wifi_init();
	
	struct net_if *iface = net_if_get_default();

	if (!iface) {
		LOG_ERR("No such interface in index %d", 1);
		return -1;
	}

	struct in_addr my_addr4;
	struct in_addr my_netmask4;

	net_addr_pton(AF_INET, CONFIG_NET_CONFIG_MY_IPV4_ADDR, &my_addr4);
	net_addr_pton(AF_INET, CONFIG_NET_CONFIG_MY_IPV4_NETMASK, &my_netmask4);

	net_if_ipv4_addr_add(iface, &my_addr4, NET_ADDR_AUTOCONF, 0);
	net_if_ipv4_set_netmask_by_addr(iface, &my_addr4, &my_netmask4);


	wifi_softAP_start();

	k_msleep(3000);

	struct in_addr base_addr;

	net_addr_pton(AF_INET, CONFIG_DHCP_SERVER_BASE_ADDR, &base_addr);
	net_dhcpv4_server_start(iface, &base_addr);

	http_server_start();

	struct app_msg msg;
	while (1) {
		k_msgq_get(&app_msgq, &msg, K_FOREVER);
		wifi_softAP_stop();
		http_server_stop();
		net_if_ipv4_addr_rm(iface, &my_addr4);
		wifi_sta_connect();
	}

	return 0;
}
