/*
 * Copyright (c) 2023 Anhui(Shenzhen) Listenai Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "csk6/csk_wifi.h"
#include "zephyr/kernel.h"

#include <stdlib.h>

#include <zephyr/logging/log.h>
#include <zephyr/net/net_context.h>
#include <zephyr/net/net_core.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/net_mgmt.h>
#include <zephyr/sys/sys_heap.h>
LOG_MODULE_REGISTER(wifi_app, LOG_LEVEL_DBG);

static csk_wifi_event_cb_t wifi_event_cb;
static struct net_mgmt_event_callback dhcp_cb;
K_SEM_DEFINE(ip_got_sem, 0, 1);

static void handler_cb(struct net_mgmt_event_callback *cb, uint32_t mgmt_event,
		       struct net_if *iface)
{
	if (mgmt_event != NET_EVENT_IPV4_DHCP_BOUND) {
		return;
	}

	char buf[NET_IPV4_ADDR_LEN];

	LOG_INF("Your address: %s",
		net_addr_ntop(AF_INET, &iface->config.dhcpv4.requested_ip, buf, sizeof(buf)));
	LOG_INF("Lease time: %u seconds", iface->config.dhcpv4.lease_time);
	LOG_INF("Router: %s", net_addr_ntop(AF_INET, &iface->config.ip.ipv4->gw, buf, sizeof(buf)));
	k_sem_give(&ip_got_sem);
}

static void wifi_event_handler(csk_wifi_event_t events, void *event_data, uint32_t data_len,
			       void *arg)
{
	if (events & CSK_WIFI_EVT_AP_STARTED) {
		LOG_INF("[WiFi softap] started");
	} else if (events & CSK_WIFI_EVT_AP_STOPPED) {
		LOG_INF("[WiFi softap] stopped");
	} else if (events & CSK_WIFI_EVT_AP_STACONNECTED) {
		LOG_INF("[WiFi softap] station joined");
	} else if (events & CSK_WIFI_EVT_AP_STADISCONNECTED) {
		LOG_INF("[WiFi softap] station left");
	}
}

int wifi_connect(void)
{
	int ret;
	uint8_t mac_addr[6] = {0};
	csk_wifi_result_t wifi_result;
	csk_wifi_mode_set(CSK_WIFI_MODE_STA, &wifi_result, K_FOREVER);
	if (wifi_result != CSK_WIFI_SUCCESS) {
		k_panic();
	}

	ret = csk_wifi_get_mac(CSK_WIFI_MODE_STA, mac_addr);
	if (ret != 0) {
		LOG_INF("wifi get mac failed, ret: %d", ret);
		return 0;
	}
	LOG_INF("wifi station mac addr: %x:%x:%x:%x:%x:%x", mac_addr[0], mac_addr[1], mac_addr[2],
		mac_addr[3], mac_addr[4], mac_addr[5]);

	csk_wifi_sta_config_t sta_config = {.ssid = "listenai.com",
					    .pwd = "Listenai@work",
					    .encryption_mode = CSK_WIFI_AUTH_WPA2_PSK};

	int retry_count = 0;

	do {
		LOG_INF("connecting to wifi: %s ...", sta_config.ssid);
		ret = csk_wifi_sta_connect(&sta_config, &wifi_result, K_FOREVER);
		if (ret == 0) {
			break;
		} else {
			if (wifi_result == CSK_WIFI_ERR_STA_FAILED) {
				retry_count++;
				LOG_INF("retry to connecting wifi ... %d", retry_count);
			} else {
				LOG_INF("AP not found or invalid password");
				return 0;
			}
		}
	} while (retry_count < 10);
	LOG_INF("wifi connected, ssid: %s, bssid: %s, channel: %d , rssi: %d", sta_config.ssid,
		sta_config.bssid, sta_config.channel, sta_config.rssi);
	net_mgmt_init_event_callback(&dhcp_cb, handler_cb, NET_EVENT_IPV4_DHCP_BOUND);
	net_mgmt_add_event_callback(&dhcp_cb);
	struct net_if *iface = net_if_get_default();
	if (!iface) {
		LOG_INF("wifi interface not available");
		return 0;
	}
	LOG_INF("dhcpv4 start");
	net_dhcpv4_start(iface);
	LOG_INF("Waiting for got IP address");
	k_sem_take(&ip_got_sem, K_FOREVER);

	return 0;
}

int wifi_softAP(void)
{
	csk_wifi_result_t wifi_result;

	csk_wifi_init();

	LOG_INF("set to softAP mode");
	csk_wifi_mode_set(CSK_WIFI_MODE_AP, &wifi_result, K_FOREVER);
	if (wifi_result != CSK_WIFI_SUCCESS) {
		k_panic();
	}

	wifi_event_cb.handler = &wifi_event_handler;
	wifi_event_cb.events = CSK_WIFI_EVT_AP_STARTED | CSK_WIFI_EVT_AP_STOPPED |
			       CSK_WIFI_EVT_AP_STACONNECTED | CSK_WIFI_EVT_AP_STADISCONNECTED |
			       CSK_WIFI_EVT_STA_CONNECTED | CSK_WIFI_EVT_STA_DISCONNECTED;
	wifi_event_cb.arg = NULL;
	csk_wifi_add_callback(&wifi_event_cb);

	csk_wifi_softap_config_t ap_config = {
		.ssid = "softap",
		.pwd = "12345678",
		.encryption_mode = CSK_WIFI_AUTH_WPA2_PSK, /* TODO: add to menuconfig */
		.bandwidth = CSK_WIFI_BW_HT40,
		.channel = 1,
		.max_connections = 3,
	};
	LOG_INF("start softAP mode");
	csk_wifi_ap_start(&ap_config, &wifi_result, K_FOREVER);

	if (wifi_result == CSK_WIFI_ERR_AP_FAILED) {
		abort();
	}
	LOG_INF("ssid: %s password: %s", ap_config.ssid, ap_config.pwd);

	return 0;
}

int wifi_softAP_stop(void)
{
	csk_wifi_result_t wifi_result;
	csk_wifi_ap_stop(&wifi_result, K_FOREVER);
	if (wifi_result != CSK_WIFI_SUCCESS) {
		k_panic();
	}

	return 0;
}
