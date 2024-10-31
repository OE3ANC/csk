/*
 * Copyright (c) 2023 Anhui(Shenzhen) Listenai Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include "csk6/csk_wifi.h"

#include <stdlib.h>

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/net/net_context.h>
#include <zephyr/net/net_core.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/net_mgmt.h>
#include <zephyr/shell/shell.h>
#include <zephyr/sys/printk.h>

LOG_MODULE_REGISTER(wifi_app, LOG_LEVEL_DBG);

enum {
	WIFI_APP_MSG_EVT_SCAN = 0,
	WIFI_APP_MSG_EVT_AP_START,
	WIFI_APP_MSG_EVT_AP_STOP,
	WIFI_APP_MSG_EVT_STA_CONNECT,
	WIFI_APP_MSG_EVT_STA_DISCONNECT,
	WIFI_APP_MSG_EVT_MAX,
};

struct wifi_app_msg {
	uint32_t evt;
	uint32_t len;
	void *data;
};

K_MSGQ_DEFINE(wifi_app_msgq, sizeof(struct wifi_app_msg), 10, 4);

static csk_wifi_event_cb_t wifi_event_cb;
static struct net_mgmt_event_callback dhcp_cb;

static void handler_cb(struct net_mgmt_event_callback *cb, uint32_t mgmt_event,
		       struct net_if *iface)
{
	if (mgmt_event != NET_EVENT_IPV4_DHCP_BOUND) {
		return;
	}

	char buf[NET_IPV4_ADDR_LEN];

	struct in_addr *ip = net_if_ipv4_get_global_addr(iface, NET_ADDR_PREFERRED);

	printk("IPv4 address: %s\n",
	       net_addr_ntop(AF_INET, ip, buf, NET_IPV4_ADDR_LEN));
	printk("Lease time: %u seconds\n", iface->config.dhcpv4.lease_time);

	struct in_addr netmask = net_if_ipv4_get_netmask_by_addr(iface, ip);
	
	printk("Subnet: %s\n",
	       net_addr_ntop(AF_INET, &netmask, buf, NET_IPV4_ADDR_LEN));

	printk("Router: %s\n",
	       net_addr_ntop(AF_INET, &iface->config.ip.ipv4->gw, buf, NET_IPV4_ADDR_LEN));
}

static void wifi_event_handler(csk_wifi_event_t events, void *event_data, uint32_t data_len,
			       void *arg)
{
	if (events & CSK_WIFI_EVT_AP_STARTED) {
		LOG_INF("wifi evt:softAP started");
	} else if (events & CSK_WIFI_EVT_AP_STOPPED) {
		LOG_INF("wifi evt: softAP stopped");
	} else if (events & CSK_WIFI_EVT_AP_STACONNECTED) {
		LOG_INF("wifi evt: station connected");
	} else if (events & CSK_WIFI_EVT_AP_STADISCONNECTED) {
		LOG_INF("wifi evt: station disconnected");
	} else if (events & CSK_WIFI_EVT_SCAN_FAILED) {
		LOG_INF("wifi evt: scan failed");
	} else if (events & CSK_WIFI_EVT_SCAN_DONE) {
		LOG_INF("wifi evt: scan done");
	} else if (events & CSK_WIFI_EVT_STA_CONNECTED) {
		LOG_INF("wifi evt: sta connected");
	} else if (events & CSK_WIFI_EVT_STA_DISCONNECTED) {
		LOG_INF("wifi evt: sta disconnected");
	} else if (events & CSK_WIFI_EVT_STA_CONNECTION_FAILED) {
		LOG_INF("wifi evt: sta connection failed");
	}
}

const char *auth_names[CSK_WIFI_AUTH_MAX] = {
	[CSK_WIFI_AUTH_OPEN] = "OPEN",
	[CSK_WIFI_AUTH_WEP] = "WEP",
	[CSK_WIFI_AUTH_WPA_PSK] = "WPA_PSK",
	[CSK_WIFI_AUTH_WPA2_PSK] = "WPA2_PSK",
	[CSK_WIFI_AUTH_WPA_WPA2_PSK] = "WPA_WPA2_PSK",
	[CSK_WIFI_AUTH_WPA2_ENTERPRISE] = "WPA2_ENTERPRISE",
	[CSK_WIFI_AUTH_WPA3_PSK] = "WPA3_PSK",
	[CSK_WIFI_AUTH_WPA2_WPA3_PSK] = "WPA2_WPA3_PSK",
};

static const char *wifi_auth_name_get_by_type(uint8_t type)
{

	if (type >= CSK_WIFI_AUTH_MAX) {
		return "unknown";
	}

	return auth_names[type];
}

static uint8_t wifi_auth_type_get_by_name(const char *name)
{
	int i;

	for (i = 0; i < CSK_WIFI_AUTH_MAX; i++) {
		if (strcmp(name, auth_names[i]) == 0) {
			return i;
		}
	}

	return CSK_WIFI_AUTH_MAX;
}

static int wifi_app_scan(void)
{
	int r;
	csk_wifi_scan_info_t *ap_info;
	csk_wifi_result_t result;

	r = csk_wifi_scan_ap(&ap_info, &result, K_FOREVER);
	if (result != CSK_WIFI_SUCCESS) {
		LOG_ERR("csk wifi scan failed, err:%d", r);
		return -EIO;
	}

	LOG_INF("wifi scan done, num:%d", r);
	int i;

	for (i = 0; i < r; i++) {
		LOG_INF("ssid:%20s, bssid:%s, rssi:%04d, ch:%02d, auth mode:%s", ap_info[i].ssid,
			ap_info[i].bssid, ap_info[i].rssi, ap_info[i].channel,
			wifi_auth_name_get_by_type(ap_info[i].encryption_mode));
	}

	csk_wifi_scan_info_free(ap_info);

	return 0;
}

static int wifi_app_ap_start(csk_wifi_softap_config_t *config)
{
	csk_wifi_result_t wifi_result;
	int r;

	r = csk_wifi_mode_set(CSK_WIFI_MODE_AP, &wifi_result, K_FOREVER);
	if (r != 0 || wifi_result != CSK_WIFI_SUCCESS) {
		return -EINVAL;
	}

	r = csk_wifi_ap_start(config, &wifi_result, K_FOREVER);
	if (r != 0 || wifi_result != CSK_WIFI_ERR_AP_FAILED) {
		return -EINVAL;
	}

	LOG_INF("Wifi softAP mode started, ssid:%s, password:%s, auth:%s", config->ssid,
		config->pwd, wifi_auth_name_get_by_type(config->encryption_mode));

	return 0;
}

static int wifi_app_ap_stop(void)
{
	csk_wifi_result_t wifi_result;

	csk_wifi_ap_stop(&wifi_result, K_FOREVER);
	if (wifi_result != CSK_WIFI_SUCCESS) {
		return -EIO;
	}

	return 0;
}

static int wifi_app_sta_connect(csk_wifi_sta_config_t *config)
{
	int r;
	csk_wifi_result_t wifi_result;

	r = csk_wifi_mode_set(CSK_WIFI_MODE_STA, &wifi_result, K_FOREVER);
	if (r != 0 || wifi_result != CSK_WIFI_SUCCESS) {
		return -EINVAL;
	}

	LOG_INF("try to connect to wifi, ssid:%s, password:%s", config->ssid, config->pwd);
	r = csk_wifi_sta_connect(config, &wifi_result, K_FOREVER);
	if (r != 0 || wifi_result != CSK_WIFI_SUCCESS) {
		LOG_ERR("failed to connect to wifi ssid:%s", config->ssid);
		return -EIO;
	}

	return 0;
}

static int wifi_app_sta_disconnect(void)
{
	int r;
	csk_wifi_result_t wifi_result;

	r = csk_wifi_sta_disconnect(&wifi_result, K_FOREVER);
	if (wifi_result != CSK_WIFI_SUCCESS) {
		return -EIO;
	}

	return 0;
}

int main(void)
{
	int r;

	r = csk_wifi_init();
	if (r) {
		LOG_ERR("csk wifi init failed, err:%d", r);
		return r;
	}

	wifi_event_cb.handler = &wifi_event_handler;
	wifi_event_cb.events = CSK_WIFI_EVT_STA_CONNECTED | CSK_WIFI_EVT_STA_DISCONNECTED |
			       CSK_WIFI_EVT_AP_STARTED | CSK_WIFI_EVT_AP_STOPPED |
			       CSK_WIFI_EVT_AP_STACONNECTED | CSK_WIFI_EVT_AP_STADISCONNECTED |
			       CSK_WIFI_EVT_SCAN_DONE | CSK_WIFI_EVT_STA_CONNECTING |
			       CSK_WIFI_EVT_STA_CONNECTION_FAILED | CSK_WIFI_EVT_SCAN_FAILED;
	wifi_event_cb.arg = NULL;
	r = csk_wifi_add_callback(&wifi_event_cb);
	if (r) {
		LOG_ERR("csk wifi callback add failed, err:%d", r);
		return r;
	}

	net_mgmt_init_event_callback(&dhcp_cb, handler_cb, NET_EVENT_IPV4_DHCP_BOUND);
	net_mgmt_add_event_callback(&dhcp_cb);

	struct wifi_app_msg msg;
	LOG_INF("Wifi Station/softAP Sample");
	while (1) {
		k_msgq_get(&wifi_app_msgq, &msg, K_FOREVER);
		LOG_INF("wifi app message received, evt:%d", msg.evt);
		switch (msg.evt) {
		case WIFI_APP_MSG_EVT_SCAN:
			wifi_app_scan();
		case WIFI_APP_MSG_EVT_AP_START: {
			struct in_addr my_addr4;
			struct in_addr my_netmask4;
			// struct net_if *iface = net_if_get_by_index(1);
			struct net_if *iface = net_if_get_default();
			if (!iface) {
				LOG_INF("No such interface in index %d", 1);
				return -1;
			}

			LOG_INF("set local ipv4 address:%s, netmask:%s",
				CONFIG_EXAMPLE_LOCAL_IPV4_ADDRESS,
				CONFIG_EXAMPLE_LOCAL_IPV4_NETMASK);
			net_addr_pton(AF_INET, CONFIG_EXAMPLE_LOCAL_IPV4_ADDRESS, &my_addr4);
			net_addr_pton(AF_INET, CONFIG_EXAMPLE_LOCAL_IPV4_NETMASK, &my_netmask4);
			net_if_ipv4_addr_rm(iface, &my_addr4);
			if (net_if_ipv4_addr_add(iface, &my_addr4, NET_ADDR_AUTOCONF, 0) == NULL) {
				LOG_INF("ipv4 addr set failed");
				return -1;
			}

			net_if_ipv4_set_netmask_by_addr(iface, &my_addr4, &my_netmask4);
			net_dhcpv4_stop(iface);
			if (msg.data != NULL) {
				wifi_app_ap_start(msg.data);
			}
		} break;
		case WIFI_APP_MSG_EVT_AP_STOP:
			wifi_app_ap_stop();
			break;
		case WIFI_APP_MSG_EVT_STA_CONNECT:
			if (msg.data != NULL) {
				if (wifi_app_sta_connect(msg.data) == 0) {
					struct net_if *iface = net_if_get_default();
					if (iface) {
						struct in_addr my_addr4;
						net_addr_pton(AF_INET,
							      CONFIG_EXAMPLE_LOCAL_IPV4_ADDRESS,
							      &my_addr4);

						net_if_ipv4_addr_rm(iface, &my_addr4);
						net_dhcpv4_restart(iface);
					} else {
						LOG_INF("wifi interface not available");
					}
				}
			}
			break;
		case WIFI_APP_MSG_EVT_STA_DISCONNECT:
			wifi_app_sta_disconnect();
			break;
		default:
			break;
		}

		if (msg.data != NULL) {
			k_free(msg.data);
		}
	}

	return 0;
}

static int shell_cmd_wifi_scan(const struct shell *sh, size_t argc, char **argv)
{
	int r;
	struct wifi_app_msg msg = {0};

	msg.evt = WIFI_APP_MSG_EVT_SCAN;
	msg.len = 0;
	r = k_msgq_put(&wifi_app_msgq, &msg, K_NO_WAIT);
	if (r) {
		shell_error(sh, "msg put failed, err:%d", r);
	}

	return 0;
}

static int shell_cmd_wifi_ap_start(const struct shell *sh, size_t argc, char **argv)
{
	if (argc < 4) {
		shell_error(sh, "invalid arg nums");
		return SHELL_ERROR;
	}

	csk_wifi_softap_config_t *config = k_malloc(sizeof(csk_wifi_softap_config_t));
	if (config == NULL) {
		shell_error(sh, "config malloc failed");
		return SHELL_ERROR;
	}
	memset(config, 0, sizeof(csk_wifi_softap_config_t));

	config->bandwidth = CSK_WIFI_BW_HT40;
	config->channel = 1;
	config->max_connections = 3;

	/* ssid */
	if (strlen(argv[1]) >= sizeof(config->ssid)) {
		shell_error(sh, "ssid is to long, must less than %dbytes, ssid:%s",
			    sizeof(config->ssid), argv[1]);
		k_free(config);
		return SHELL_ERROR;
	}
	memcpy(config->ssid, argv[1], strlen(argv[1]));

	/* password */
	if (strlen(argv[2]) >= sizeof(config->pwd)) {
		shell_error(sh, "ssid is to long, must less than %dbytes, pwd:%s",
			    sizeof(config->pwd), argv[2]);
		k_free(config);
		return SHELL_ERROR;
	}
	memcpy(config->pwd, argv[2], strlen(argv[2]));

	/* auth mode */
	config->encryption_mode = wifi_auth_type_get_by_name(argv[3]);
	if (config->encryption_mode >= CSK_WIFI_AUTH_MAX) {
		shell_error(sh, "invalid encryption mode:%s", argv[3]);
		k_free(config);
		return SHELL_ERROR;
	}

	int r;
	struct wifi_app_msg msg;

	msg.evt = WIFI_APP_MSG_EVT_AP_START;
	msg.data = config;
	msg.len = sizeof(csk_wifi_softap_config_t);

	r = k_msgq_put(&wifi_app_msgq, &msg, K_NO_WAIT);
	if (r) {
		shell_error(sh, "msg put failed, err:%d", r);
	}

	return 0;
}

static int shell_cmd_wifi_ap_stop(const struct shell *sh, size_t argc, char **argv)
{
	int r;
	struct wifi_app_msg msg = {0};

	msg.evt = WIFI_APP_MSG_EVT_AP_STOP;
	msg.len = sizeof(csk_wifi_softap_config_t);

	r = k_msgq_put(&wifi_app_msgq, &msg, K_NO_WAIT);
	if (r) {
		shell_error(sh, "msg put failed, err:%d", r);
	}

	return 0;
}

static int shell_cmd_sta_conn(const struct shell *sh, size_t argc, char **argv)
{
	csk_wifi_sta_config_t *config = k_malloc(sizeof(csk_wifi_sta_config_t));
	if (!config) {
		shell_error(sh, "config malloc failed");
		return SHELL_ERROR;
	}

	memset(config, 0, sizeof(csk_wifi_sta_config_t));

	/* ssid */
	if (strlen(argv[1]) >= sizeof(config->ssid)) {
		shell_error(sh, "ssid is to long, must less than %dbytes, ssid:%s",
			    sizeof(config->ssid), argv[1]);
		k_free(config);
		return SHELL_ERROR;
	}
	memcpy(config->ssid, argv[1], strlen(argv[1]));

	/* password */
	if (strlen(argv[2]) >= sizeof(config->pwd)) {
		shell_error(sh, "ssid is to long, must less than %dbytes, pwd:%s",
			    sizeof(config->pwd), argv[2]);
		k_free(config);
		return SHELL_ERROR;
	}
	memcpy(config->pwd, argv[2], strlen(argv[2]));

	/* auth mode */
	config->encryption_mode = wifi_auth_type_get_by_name(argv[3]);
	if (config->encryption_mode >= CSK_WIFI_AUTH_MAX) {
		shell_error(sh, "invalid encryption mode:%s", argv[3]);
		k_free(config);
		return SHELL_ERROR;
	}

	int r;
	struct wifi_app_msg msg = {0};

	msg.evt = WIFI_APP_MSG_EVT_STA_CONNECT;
	msg.data = config;
	msg.len = sizeof(csk_wifi_sta_config_t);

	r = k_msgq_put(&wifi_app_msgq, &msg, K_NO_WAIT);
	if (r) {
		shell_error(sh, "msg put failed, err:%d", r);
	}

	return 0;
}

static int shell_cmd_sta_disconnect(const struct shell *sh, size_t argc, char **argv)
{
	int r;
	struct wifi_app_msg msg = {0};

	msg.evt = WIFI_APP_MSG_EVT_STA_DISCONNECT;
	msg.data = NULL;
	msg.len = sizeof(csk_wifi_softap_config_t);

	r = k_msgq_put(&wifi_app_msgq, &msg, K_NO_WAIT);
	if (r) {
		shell_error(sh, "msg put failed, err:%d", r);
	}

	return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(wifi_sta_cmds,
			       SHELL_CMD_ARG(conn, NULL,
					     "Start a WIFI connection request"
					     "eg.\n"
					     "wifi sta conn <ssid> <password> <auth>\n"
					     "auth:OPEN,WEP,WPA_PSK,WPA2_PSK,WPA_WPA2_PSK,WPA2_"
					     "ENTERPRISE,WPA3_PSK,WPA2_WPA3_PSK",
					     shell_cmd_sta_conn, 4, 0),
			       SHELL_CMD(disconn, NULL, "Start a WIFI disconnection request.",
					 shell_cmd_sta_disconnect),
			       SHELL_SUBCMD_SET_END /* Array terminated. */
);

SHELL_STATIC_SUBCMD_SET_CREATE(wifi_ap_cmds,
			       SHELL_CMD_ARG(start, NULL,
					     "Wifi softAP start.\n"
					     "eg.\n"
					     "wifi ap start <ssid> <password> <auth>\n"
					     "auth:OPEN,WEP,WPA_PSK,WPA2_PSK,WPA_WPA2_PSK,WPA2_"
					     "ENTERPRISE,WPA3_PSK,WPA2_WPA3_PSK",
					     shell_cmd_wifi_ap_start, 4, 0),
			       SHELL_CMD(stop, NULL, "Wifi softAP stop.", shell_cmd_wifi_ap_stop),
			       SHELL_SUBCMD_SET_END /* Array terminated. */
);

SHELL_STATIC_SUBCMD_SET_CREATE(sub_wifi_cmd,
			       SHELL_CMD(sta, &wifi_sta_cmds, "Wifi station commands.", NULL),
			       SHELL_CMD(ap, &wifi_ap_cmds, "Wifi softAP commands.", NULL),
			       SHELL_CMD(scan, NULL, "Scan the neighboring AP device.",
					 shell_cmd_wifi_scan),
			       SHELL_SUBCMD_SET_END /* Array terminated. */
);

SHELL_CMD_REGISTER(wifi, &sub_wifi_cmd, "wifi commands", NULL);
