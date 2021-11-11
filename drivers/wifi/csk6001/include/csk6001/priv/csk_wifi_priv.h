#pragma once

#include <stdio.h>
#include <assert.h>
#include <stdlib.h>

#include <kernel.h>
#include <net/ethernet.h>
#include <net/net_pkt.h>
#include <net/net_if.h>

#include "spi_drv.h"
#include "netdev_api.h"
#include "hosted/hosted.h"
#include "csk6001/csk_wifi.h"

#ifdef __cplusplus
extern "C" {
#endif

struct csk_wifi_data {
    const struct device *dev;
    struct network_handle *virtual_netif;
    struct net_if *netif;
    uint8_t mac_addr[6];
    union {
        esp_hosted_sta_config_t sta_config;
        esp_hosted_softap_config_t ap_config;
    };
    hosted_event_callback_t hosted_event_cb;
};

static struct csk_wifi_data csk_wifi_sta_data;
static struct csk_wifi_data csk_wifi_ap_data;

typedef enum {
    WIFI_CMD_STA_INIT,
    WIFI_CMD_STA_CONNECT,
    WIFI_CMD_STA_DISCONNECT,
    WIFI_CMD_AP_INIT,
    WIFI_CMD_AP_START,
    WIFI_CMD_AP_STOP,
} wifi_cmd_t;

const struct device * wifi_get_dev(wifi_mode_t mode);
int wifi_send_command(const struct device *dev, wifi_cmd_t command, k_timeout_t timeouts);
int wifi_take_result(csk_wifi_result_t *result, k_timeout_t timeout);

void wifi_dispatch_callbacks(csk_wifi_event_t events);

#ifdef __cplusplus
}
#endif
